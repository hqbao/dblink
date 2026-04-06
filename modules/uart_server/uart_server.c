#include "uart_server.h"
#include <string.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "pubsub.h"
#include "messages.h"
#include "platform.h"

#define TAG "uart_server"

#define FC_UART_PORT  UART_NUM_1
#define FC_BAUD_RATE  19200

static TaskHandle_t g_rx_task_handle = NULL;

// ---------------------------------------------------------------------------
// UDP/USB → UART: forward received bytes to flight controller
// ---------------------------------------------------------------------------
static void on_packet_to_uart(uint8_t *data, size_t size) {
    if (size < sizeof(raw_packet_t)) return;
    raw_packet_t *pkt = (raw_packet_t *)data;
    if (!pkt->data || pkt->len == 0) return;

    led_send();
    uart_write_bytes(FC_UART_PORT, (const char *)pkt->data, pkt->len);
}

// ---------------------------------------------------------------------------
// UART RX: forward raw bytes, publish UART_RECEIVED
// ---------------------------------------------------------------------------
static void uart_rx_task(void *arg) {
    uint8_t rx_buf[256];

    while (1) {
        int len = uart_read_bytes(FC_UART_PORT, rx_buf, sizeof(rx_buf),
                                  20 / portTICK_PERIOD_MS);
        if (len <= 0) continue;

        led_recv();
        raw_packet_t pkt = { .data = rx_buf, .len = (size_t)len };
        publish(UART_RECEIVED, (uint8_t *)&pkt, sizeof(raw_packet_t));
    }
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void uart_server_setup(void) {

    const uart_config_t cfg = {
        .baud_rate  = FC_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_param_config(FC_UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_driver_install(FC_UART_PORT, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_set_pin(FC_UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    subscribe(UDP_RECEIVED, on_packet_to_uart);

    xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 10, &g_rx_task_handle);

    ESP_LOGI(TAG, "UART%d @ %d baud", FC_UART_PORT, FC_BAUD_RATE);
}
