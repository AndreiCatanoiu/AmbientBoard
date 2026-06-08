#include "wifi_manager.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "WIFI_MGR";

#define CONNECTED_BIT     BIT0
#define DISCONNECTED_BIT  BIT1

#define WIFI_CONNECT_TIMEOUT_MS 10000
#define WIFI_TASK_PERIOD_MS     1000

typedef enum
{
    WIFI_STATE_INVALID = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTING,
    WIFI_STATE_DISCONNECTED,
} wifi_state_t;

static EventGroupHandle_t s_wifi_event_group = NULL;
static wifi_state_t s_state = WIFI_STATE_INVALID;
static bool s_initialized = false;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Station pornit");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Deconectat de la AP");
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, DISCONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "IP primit: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupClearBits(s_wifi_event_group, DISCONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    }
}

esp_err_t wifi_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "WiFi deja initializat");
        return ESP_OK;
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Nu pot crea event group");
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_state = WIFI_STATE_DISCONNECTED;
    s_initialized = true;
    ESP_LOGI(TAG, "Init WiFi finalizat (SSID: %s)", WIFI_SSID);
    return ESP_OK;
}

bool wifi_is_connected(void)
{
    return s_state == WIFI_STATE_CONNECTED;
}

void wifi_task(void *pvParameters)
{
    (void) pvParameters;

    if (!s_initialized) {
        ESP_LOGE(TAG, "Apeleaza wifi_init() inainte de a porni wifi_task");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "wifi_task pornit");
    EventBits_t bits;

    for (;;) {
        switch (s_state) {
        case WIFI_STATE_DISCONNECTED:
            ESP_LOGI(TAG, "Ma conectez la %s...", WIFI_SSID);
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
                ESP_LOGW(TAG, "Conectarea a esuat, reincerc");
                s_state = WIFI_STATE_DISCONNECTED;
            }
            break;

        case WIFI_STATE_CONNECTED:
            bits = xEventGroupGetBits(s_wifi_event_group);
            if (bits & DISCONNECTED_BIT) {
                ESP_LOGW(TAG, "Conexiunea a picat");
                s_state = WIFI_STATE_DISCONNECTING;
            }
            break;

        case WIFI_STATE_DISCONNECTING:
            esp_wifi_connect();
            bits = xEventGroupWaitBits(s_wifi_event_group,
                                       CONNECTED_BIT | DISCONNECTED_BIT,
                                       pdFALSE, pdFALSE,
                                       pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));
            if (bits & CONNECTED_BIT) {
                ESP_LOGI(TAG, "Reconectat");
                s_state = WIFI_STATE_CONNECTED;
            } else {
                s_state = WIFI_STATE_DISCONNECTED;
            }
            break;

        default:
            s_state = WIFI_STATE_DISCONNECTED;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(WIFI_TASK_PERIOD_MS));
    }
}
