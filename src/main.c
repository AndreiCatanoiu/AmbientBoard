#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "settings_store.h"
#include "app_state.h"
#include "display.h"
#include "ui.h"
#include "rgb_led.h"

#include "wifi_manager.h"
#include "time_manager.h"
#include "senzor_temperatura.h"
#include "mqtt_comm.h"
#include "ota_update.h"

void app_main(void)
{
    settings_init();
    app_state_init();
    rgb_led_init();

    uint32_t led = settings_get_led();
    rgb_led_set((led >> 16) & 0xFF, (led >> 8) & 0xFF, led & 0xFF);

    display_init();
    ui_init();

    ESP_ERROR_CHECK(wifi_init());
    time_init();
    temp_sensor_init();
    mqtt_comm_init();
    ota_update_init();

    xTaskCreate(&display_task, "display_task", 12288, NULL, 4, NULL);
    xTaskCreate(&wifi_task, "wifi_task", 4096, NULL, 5, NULL);
    xTaskCreate(&time_task, "time_task", 4096, NULL, 5, NULL);
    xTaskCreate(&temp_sensor_task, "temp_sensor_task", 4096, NULL, 5, NULL);

    ESP_LOGI("APP_MAIN", "AmbientBoard pornit");
}
