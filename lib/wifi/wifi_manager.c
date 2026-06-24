#include "wifi_manager.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "settings_store.h"
#include "app_state.h"

static const char *TAG = "WIFI_MGR";

#define CONNECTED_BIT     BIT0
#define DISCONNECTED_BIT  BIT1

#define WIFI_CONNECT_TIMEOUT_MS 10000
#define WIFI_TASK_PERIOD_MS     1000

typedef enum {
    WIFI_STATE_INVALID = 0,
    WIFI_STATE_IDLE,          
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTING,
    WIFI_STATE_DISCONNECTED,
} wifi_state_t;

static EventGroupHandle_t s_wifi_event_group = NULL;
static wifi_state_t s_state = WIFI_STATE_INVALID;
static bool s_initialized = false;
static volatile bool s_reconfigure = false;
static volatile bool s_disconnect_req = false;
static volatile bool s_enabled = false;   

static char s_ssid[33] = {0};
static char s_pass[65] = {0};
static char s_ip[16] = "0.0.0.0";

static uint8_t rssi_to_strength(int8_t rssi)
{
    if (rssi <= -100) return 0;
    if (rssi >= -50)  return 100;
    return (uint8_t)(2 * (rssi + 100));
}

static void apply_config(void)
{
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, s_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, s_pass, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Station pornit");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Deconectat de la AP");
        strcpy(s_ip, "0.0.0.0");
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, DISCONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "IP primit: %s", s_ip);
        xEventGroupClearBits(s_wifi_event_group, DISCONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    }
}

esp_err_t wifi_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    char ssid[33] = {0};
    char pass[65] = {0};
    if (settings_get_wifi(ssid, pass)) {
        strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
        strncpy(s_pass, pass, sizeof(s_pass) - 1);
        s_enabled = true;
        ESP_LOGI(TAG, "Folosesc credentiale din NVS (SSID: %s)", s_ssid);
    } else {
        s_enabled = false;
        ESP_LOGI(TAG, "Nicio retea salvata - astept configurare din interfata");
    }

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    apply_config();
    ESP_ERROR_CHECK(esp_wifi_start());

    s_state = s_enabled ? WIFI_STATE_DISCONNECTED : WIFI_STATE_IDLE;
    s_initialized = true;
    ESP_LOGI(TAG, "Init WiFi finalizat");
    return ESP_OK;
}

bool wifi_is_connected(void)
{
    return s_state == WIFI_STATE_CONNECTED;
}

uint8_t wifi_scan(wifi_scan_entry_t *entries, uint8_t max)
{
    if (!s_initialized || entries == NULL || max == 0) {
        return 0;
    }

    wifi_scan_config_t scan_cfg = {0};
    if (esp_wifi_scan_start(&scan_cfg, true) != ESP_OK) {
        return 0;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count == 0) {
        return 0;
    }
    if (ap_count > WIFI_SCAN_MAX) {
        ap_count = WIFI_SCAN_MAX;
    }

    static wifi_ap_record_t records[WIFI_SCAN_MAX];
    esp_wifi_scan_get_ap_records(&ap_count, records);

    uint8_t n = (ap_count < max) ? (uint8_t)ap_count : max;
    for (uint8_t i = 0; i < n; i++) {
        snprintf(entries[i].ssid, sizeof(entries[i].ssid), "%s", (char *)records[i].ssid);
        entries[i].strength = rssi_to_strength(records[i].rssi);
        entries[i].open = (records[i].authmode == WIFI_AUTH_OPEN) ? 1 : 0;
    }
    return n;
}

void wifi_connect_to(const char *ssid, const char *pass)
{
    if (ssid == NULL || ssid[0] == '\0') {
        return;
    }
    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';
    strncpy(s_pass, pass ? pass : "", sizeof(s_pass) - 1);
    s_pass[sizeof(s_pass) - 1] = '\0';

    settings_set_wifi(s_ssid, s_pass);
    s_enabled = true;
    s_reconfigure = true;
    ESP_LOGI(TAG, "Cerere conectare la retea noua: %s", s_ssid);
}

void wifi_disconnect(void)
{
    s_enabled = false;
    s_disconnect_req = true;
    ESP_LOGI(TAG, "Cerere deconectare WiFi");
}

void wifi_reconnect(void)
{
    if (s_ssid[0] == '\0') {
        ESP_LOGW(TAG, "Nicio retea salvata pentru reconectare");
        return;
    }
    s_enabled = true;
    ESP_LOGI(TAG, "Cerere reconectare la reteaua salvata: %s", s_ssid);
}

bool wifi_has_saved(void)
{
    return s_ssid[0] != '\0';
}

void wifi_get_ip_string(char *out, uint8_t len)
{
    snprintf(out, len, "%s", s_ip);
}

uint8_t wifi_get_strength(void)
{
    wifi_ap_record_t info;
    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        return rssi_to_strength(info.rssi);
    }
    return 0;
}

void wifi_get_ssid(char *out, uint8_t len)
{
    snprintf(out, len, "%s", s_ssid);
}

static void publish_state(void)
{
    app_wifi_t w = {0};
    w.connected = wifi_is_connected() ? 1 : 0;
    w.strength = w.connected ? wifi_get_strength() : 0;
    snprintf(w.ip, sizeof(w.ip), "%s", s_ip);
    snprintf(w.ssid, sizeof(w.ssid), "%s", s_ssid);
    app_state_set_wifi(&w);
}

void wifi_task(void *pvParameters)
{
    (void) pvParameters;
    if (!s_initialized) {
        ESP_LOGE(TAG, "Apeleaza wifi_init() inainte de wifi_task");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "wifi_task pornit");
    EventBits_t bits;

    for (;;) {
        if (s_reconfigure) {
            s_reconfigure = false;
            ESP_LOGI(TAG, "Reconfigurez WiFi cu reteaua noua");
            esp_wifi_disconnect();
            apply_config();
            s_state = WIFI_STATE_DISCONNECTED;
        }

        if (s_disconnect_req) {
            s_disconnect_req = false;
            ESP_LOGI(TAG, "Deconectez WiFi (la cerere)");
            esp_wifi_disconnect();
            strcpy(s_ip, "0.0.0.0");
            s_state = WIFI_STATE_IDLE;
        }

        switch (s_state) {
        case WIFI_STATE_IDLE:
            if (s_enabled) {
                s_state = WIFI_STATE_DISCONNECTED;
            }
            break;

        case WIFI_STATE_DISCONNECTED:
            if (!s_enabled) {
                s_state = WIFI_STATE_IDLE;
                break;
            }
            s_state = WIFI_STATE_CONNECTING;
            xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT | DISCONNECTED_BIT);
            esp_wifi_connect();
            break;

        case WIFI_STATE_CONNECTING:
            bits = xEventGroupWaitBits(s_wifi_event_group,
                                       CONNECTED_BIT | DISCONNECTED_BIT,
                                       pdFALSE, pdFALSE,
                                       pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));
            if (bits & CONNECTED_BIT) {
                ESP_LOGI(TAG, "Conectat");
                s_state = WIFI_STATE_CONNECTED;
            } else {
                s_state = WIFI_STATE_DISCONNECTED;
            }
            break;

        case WIFI_STATE_CONNECTED:
            bits = xEventGroupGetBits(s_wifi_event_group);
            if (bits & DISCONNECTED_BIT) {
                s_state = WIFI_STATE_DISCONNECTING;
            }
            break;

        case WIFI_STATE_DISCONNECTING:
            if (!s_enabled) {
                s_state = WIFI_STATE_IDLE;
                break;
            }
            esp_wifi_connect();
            bits = xEventGroupWaitBits(s_wifi_event_group,
                                       CONNECTED_BIT | DISCONNECTED_BIT,
                                       pdFALSE, pdFALSE,
                                       pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));
            if (bits & CONNECTED_BIT) {
                s_state = WIFI_STATE_CONNECTED;
            } else {
                s_state = WIFI_STATE_DISCONNECTED;
            }
            break;

        default:
            s_state = WIFI_STATE_DISCONNECTED;
            break;
        }

        publish_state();
        vTaskDelay(pdMS_TO_TICKS(WIFI_TASK_PERIOD_MS));
    }
}