#include "mqtt_comm.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "mqtt_client.h"

#include "remote_cfg.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t s_client = NULL;
static SemaphoreHandle_t s_mutex = NULL;
static volatile bool s_connected = false;
static volatile uint32_t s_version = 0;

static char s_messages[MQTT_MSG_MAX][MQTT_MSG_LEN];
static uint8_t s_count = 0;

static void add_message(const char *text)
{
    if (s_mutex == NULL) {
        return;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_count < MQTT_MSG_MAX) {
        snprintf(s_messages[s_count], MQTT_MSG_LEN, "%s", text);
        s_count++;
    } else {
        for (uint8_t i = 1; i < MQTT_MSG_MAX; i++) {
            memcpy(s_messages[i - 1], s_messages[i], MQTT_MSG_LEN);
        }
        snprintf(s_messages[MQTT_MSG_MAX - 1], MQTT_MSG_LEN, "%s", text);
    }
    s_version++;
    xSemaphoreGive(s_mutex);
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
        esp_mqtt_client_subscribe(s_client, MQTT_TOPIC_IN, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "Deconectat de la broker");
        break;
    case MQTT_EVENT_DATA: {
        char buf[MQTT_MSG_LEN];
        int len = event->data_len < (int)(sizeof(buf) - 1) ? event->data_len
                                                            : (int)(sizeof(buf) - 1);
        memcpy(buf, event->data, len);
        buf[len] = '\0';
        ESP_LOGI(TAG, "Mesaj primit: %s", buf);
        add_message(buf);
        break;
    }
    default:
        break;
    }
}

void mqtt_comm_init(void)
{
    s_mutex = xSemaphoreCreateMutex();

    static char s_broker_uri[80];
    if (remote_build_mqtt_uri(s_broker_uri, sizeof(s_broker_uri)) == 0) {
        ESP_LOGE(TAG, "Nu pot rezolva adresa brokerului");
        return;
    }

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = s_broker_uri,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
    };
    s_client = esp_mqtt_client_init(&cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "Nu pot crea clientul MQTT");
        return;
    }
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
    ESP_LOGI(TAG, "Client MQTT pornit (TLS)");
}

bool mqtt_is_connected(void)
{
    return s_connected;
}

void mqtt_publish_message(const char *text)
{
    if (s_client == NULL || text == NULL || text[0] == '\0') {
        return;
    }
    esp_mqtt_client_publish(s_client, MQTT_TOPIC_OUT, text, 0, 1, 0);

    char line[MQTT_MSG_LEN];
    snprintf(line, sizeof(line), "> %s", text);
    add_message(line);
}

uint8_t mqtt_get_message_count(void)
{
    return s_count;
}

void mqtt_get_message(uint8_t index, char *out, uint8_t len)
{
    if (s_mutex == NULL || index >= s_count) {
        if (len > 0) out[0] = '\0';
        return;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    snprintf(out, len, "%s", s_messages[index]);
    xSemaphoreGive(s_mutex);
}

uint32_t mqtt_get_version(void)
{
    return s_version;
}
