#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_manager.h"
#include "time_manager.h"
#include "senzor_temperatura.h"

void app_main(void)
{
    ESP_ERROR_CHECK(wifi_init());
    time_init();
    temp_sensor_init();

    xTaskCreate(&wifi_task, "wifi_task", 4096, NULL, 5, NULL);
    xTaskCreate(&time_task, "time_task", 4096, NULL, 5, NULL);
    xTaskCreate(&temp_sensor_task, "temp_sensor_task", 4096, NULL, 5, NULL);

    while (!wifi_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI("APP_MAIN", "WiFi conectat, continui cu restul aplicatiei");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
