#include "mqtt_comm.h"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "mqtt_client.h"

#include "mqtt_ca_pem.h"
#include "remote_cfg.h"
#include "settings_store.h"
#include "inbox.h"

static const char *TAG = "MQTT";

#define MQTT_QOS  1

static esp_mqtt_client_handle_t s_client = NULL;
static volatile bool s_connected = false;

static char s_broker_uri[80];
static char s_mqtt_user[16];
static char s_mqtt_pass[16];
static char s_client_id[32];

static void build_client_id(char *out, size_t len)
{
    char name[MQTT_DEVICE_NAME_LEN];
    if (settings_get_mqtt_name(name, sizeof(name))) {
        snprintf(out, len, "amb_%s", name);
        return;
    }
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(out, len, "amb_%02X%02X%02X", mac[3], mac[4], mac[5]);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    (void) handler_args;
    (void) base;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "Conectat la broker");
        inbox_refresh();
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "Deconectat de la broker");
        break;
    case MQTT_EVENT_ERROR:
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG, "Eroare TLS: %s",
                     esp_err_to_name(event->error_handle->esp_tls_last_esp_err));
        }
        break;
    default:
        break;
    }
}

static bool mqtt_start_client(void)
{
    if (remote_build_mqtt_uri(s_broker_uri, sizeof(s_broker_uri)) == 0) {
        return false;
    }
    if (remote_get_mqtt_user(s_mqtt_user, sizeof(s_mqtt_user)) == 0 ||
        remote_get_mqtt_pass(s_mqtt_pass, sizeof(s_mqtt_pass)) == 0) {
        return false;
    }

    build_client_id(s_client_id, sizeof(s_client_id));

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = s_broker_uri,
        .broker.verification.certificate = MQTT_CA_CERT_PEM,
        .credentials.username = s_mqtt_user,
        .credentials.authentication.password = s_mqtt_pass,
        .credentials.client_id = s_client_id,
    };

    s_client = esp_mqtt_client_init(&cfg);
    if (s_client == NULL) {
        return false;
    }
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (esp_mqtt_client_start(s_client) != ESP_OK) {
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        return false;
    }
    return true;
}

static void mqtt_stop_client(void)
{
    if (s_client == NULL) {
        return;
    }
    esp_mqtt_client_stop(s_client);
    esp_mqtt_client_destroy(s_client);
    s_client = NULL;
    s_connected = false;
}

void mqtt_comm_init(void)
{
    mqtt_start_client();
}

void mqtt_comm_reconnect(void)
{
    mqtt_stop_client();
    mqtt_start_client();
}

bool mqtt_is_connected(void)
{
    return s_connected;
}

bool mqtt_has_device_name(void)
{
    char name[MQTT_DEVICE_NAME_LEN];
    return settings_get_mqtt_name(name, sizeof(name));
}

void mqtt_publish_message(const char *text)
{
    if (s_client == NULL || !s_connected || text == NULL || text[0] == '\0') {
        return;
    }

    char my_name[MQTT_DEVICE_NAME_LEN];
    if (!settings_get_mqtt_name(my_name, sizeof(my_name))) {
        return;
    }

    char topic[48];
    snprintf(topic, sizeof(topic), "%s/%s", MQTT_TOPIC_PREFIX, my_name);
    esp_mqtt_client_publish(s_client, topic, text, 0, MQTT_QOS, 0);
    ESP_LOGI(TAG, "Trimis");
    inbox_refresh();
}
