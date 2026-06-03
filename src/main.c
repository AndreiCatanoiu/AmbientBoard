#include "esp_err.h"
#include "esp_log.h"

void app_main(void)
{
    while (1) {
        ESP_LOGI("APP_MAIN", "Hello, ESP32!");
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }      
}