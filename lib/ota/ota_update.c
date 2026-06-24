#include "ota_update.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#include "remote_cfg.h"
#include "wifi_manager.h"

#ifndef CURRENT_FW_VERSION
#define CURRENT_FW_VERSION "v0.0.0"
#endif

#define OTA_MIN_IMAGE_BYTES  400000

static const char *TAG = "OTA";

static volatile ota_state_t s_state = OTA_STATE_IDLE;
static char s_status[64] = "Inactiv";
static char s_latest_version[16] = "--";
static char s_latest_bin[64] = "";
static volatile bool s_update_available = false;
static TaskHandle_t s_task = NULL;
static SemaphoreHandle_t s_lock = NULL;

static void set_status(ota_state_t state, const char *text)
{
    s_state = state;
    if (text != NULL) {
        snprintf(s_status, sizeof(s_status), "%s", text);
    }
}

static int parse_version_triplet(const char *s, int *maj, int *min, int *pat)
{
    if (s == NULL) {
        return -1;
    }
    while (*s != '\0' && !isdigit((unsigned char)*s)) {
        s++;
    }
    return (sscanf(s, "%d.%d.%d", maj, min, pat) == 3) ? 0 : -1;
}

static bool version_is_newer(const char *latest, const char *current)
{
    int lma, lmi, lpa;
    int cma, cmi, cpa;
    if (parse_version_triplet(latest, &lma, &lmi, &lpa) != 0) {
        return false;
    }
    if (parse_version_triplet(current, &cma, &cmi, &cpa) != 0) {
        return false;
    }
    if (lma != cma) {
        return lma > cma;
    }
    if (lmi != cmi) {
        return lmi > cmi;
    }
    return lpa > cpa;
}

static bool json_extract_string(const char *json, const char *key, char *out, size_t out_len)
{
    if (json == NULL || key == NULL || out == NULL || out_len == 0) {
        return false;
    }

    char needle[24];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (p == NULL) {
        return false;
    }

    p += strlen(needle);
    while (*p == ' ' || *p == '\t' || *p == ':') {
        p++;
    }
    if (*p != '"') {
        return false;
    }
    p++;

    size_t i = 0;
    while (*p != '\0' && *p != '"' && i < out_len - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return i > 0;
}

static bool bin_name_valid(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    if (strchr(name, '/') != NULL || strchr(name, '\\') != NULL) {
        return false;
    }
    if (strstr(name, "..") != NULL) {
        return false;
    }
    return true;
}

static bool http_get_body(const char *url, char *body, size_t body_len)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        return false;
    }

    bool ok = false;
    if (esp_http_client_open(client, 0) == ESP_OK) {
        int total = esp_http_client_fetch_headers(client);
        if (total < 0) {
            total = (int)body_len - 1;
        }
        if (total > (int)body_len - 1) {
            total = (int)body_len - 1;
        }
        int read = esp_http_client_read(client, body, total);
        if (read > 0) {
            body[read] = '\0';
            ok = true;
        }
    }
    esp_http_client_cleanup(client);
    return ok;
}

static bool ota_image_size_ok(const char *url)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        return false;
    }

    esp_http_client_set_method(client, HTTP_METHOD_HEAD);
    esp_err_t err = esp_http_client_perform(client);
    int len = esp_http_client_get_content_length(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HEAD esuat, continui fara verificare marime");
        return true;
    }

    if (len < 0) {
        ESP_LOGW(TAG, "Marime necunoscuta, continui");
        return true;
    }

    ESP_LOGI(TAG, "Marime bin OTA: %d bytes", len);
    if (len < OTA_MIN_IMAGE_BYTES) {
        ESP_LOGE(TAG, "Bin prea mic (%d) — refuz update", len);
        return false;
    }
    return true;
}

static bool fetch_latest_manifest(void)
{
    char base[96];
    if (remote_build_ota_url(base, sizeof(base)) == 0) {
        return false;
    }

    char url[128];
    snprintf(url, sizeof(url), "%s/latest.json", base);

    char body[256] = {0};
    if (!http_get_body(url, body, sizeof(body))) {
        return false;
    }

    char version[16] = {0};
    char bin[64] = {0};
    if (!json_extract_string(body, "version", version, sizeof(version))) {
        return false;
    }
    if (!json_extract_string(body, "bin", bin, sizeof(bin)) || !bin_name_valid(bin)) {
        return false;
    }

    strncpy(s_latest_version, version, sizeof(s_latest_version) - 1);
    s_latest_version[sizeof(s_latest_version) - 1] = '\0';
    strncpy(s_latest_bin, bin, sizeof(s_latest_bin) - 1);
    s_latest_bin[sizeof(s_latest_bin) - 1] = '\0';
    s_update_available = version_is_newer(s_latest_version, CURRENT_FW_VERSION);
    return true;
}

static void version_check_task(void *arg)
{
    (void) arg;

    set_status(OTA_STATE_CHECKING, "Verific versiune...");

    if (!fetch_latest_manifest()) {
        snprintf(s_latest_version, sizeof(s_latest_version), "--");
        s_latest_bin[0] = '\0';
        s_update_available = false;
        set_status(OTA_STATE_IDLE, "Verificare esuata");
    } else if (s_update_available) {
        set_status(OTA_STATE_IDLE, "Update disponibil");
    } else {
        set_status(OTA_STATE_IDLE, "La zi");
    }

    s_task = NULL;
    vTaskDelete(NULL);
}

static void ota_task(void *arg)
{
    (void) arg;

    if (s_latest_bin[0] == '\0' && !fetch_latest_manifest()) {
        set_status(OTA_STATE_FAILED, "Manifest invalid");
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    if (!s_update_available) {
        set_status(OTA_STATE_FAILED, "Deja la zi");
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    char base[96];
    if (remote_build_ota_url(base, sizeof(base)) == 0) {
        set_status(OTA_STATE_FAILED, "Config invalida");
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    char url[160];
    snprintf(url, sizeof(url), "%s/%s", base, s_latest_bin);

    if (!ota_image_size_ok(url)) {
        set_status(OTA_STATE_FAILED, "Bin invalid (prea mic)");
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    set_status(OTA_STATE_CHECKING, "Pornesc update...");
    ESP_LOGI(TAG, "Descarc %s", url);

    esp_http_client_config_t http_cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    set_status(OTA_STATE_DOWNLOADING, "Descarc firmware...");

    esp_err_t err = esp_https_ota(&ota_cfg);
    if (err == ESP_OK) {
        set_status(OTA_STATE_DONE, "Update reusit, repornesc...");
        ESP_LOGI(TAG, "Firmware actualizat");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        ESP_LOGW(TAG, "Update esuat: %s", esp_err_to_name(err));
        set_status(OTA_STATE_FAILED, "Update esuat");
    }

    s_task = NULL;
    vTaskDelete(NULL);
}

static bool start_task(TaskFunction_t fn, const char *name)
{
    if (s_lock == NULL) {
        ota_update_init();
    }

    if (xSemaphoreTake(s_lock, 0) != pdTRUE) {
        return false;
    }

    if (s_task != NULL) {
        xSemaphoreGive(s_lock);
        return false;
    }

    if (!wifi_is_connected()) {
        set_status(OTA_STATE_FAILED, "Fara WiFi");
        xSemaphoreGive(s_lock);
        return false;
    }

    BaseType_t ok = xTaskCreate(fn, name, 8192, NULL, 5, &s_task);
    if (ok != pdPASS) {
        s_task = NULL;
        set_status(OTA_STATE_FAILED, "Task esuat");
        xSemaphoreGive(s_lock);
        return false;
    }

    xSemaphoreGive(s_lock);
    return true;
}

void ota_update_init(void)
{
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
    }
}

bool ota_check_version(void)
{
    return start_task(version_check_task, "ota_ver_task");
}

bool ota_update_start(void)
{
    return start_task(ota_task, "ota_task");
}

ota_state_t ota_get_state(void)
{
    return s_state;
}

const char *ota_get_status_text(void)
{
    return s_status;
}

const char *ota_get_latest_version(void)
{
    return s_latest_version;
}

bool ota_is_update_available(void)
{
    return s_update_available;
}
