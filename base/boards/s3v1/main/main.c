#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_log.h>

#include "platform.h"
#include "pubsub.h"
#include "wifi.h"
#include "udp_server.h"
#include "uart_server.h"

#define TAG "main"

void app_main(void) {
#if UART_USE_USB
    // Suppress all log output — USB-CDC shares the same port as our data stream
    esp_log_level_set("*", ESP_LOG_NONE);
#endif

    ESP_LOGI(TAG, "Starting Flight Streamer...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize modules
    ESP_LOGI(TAG, "Initializing UDP Server...");
    udp_server_setup();

    ESP_LOGI(TAG, "Initializing UART Server...");
    uart_server_setup();

    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_setup();

    ESP_LOGI(TAG, "System Started");
}
