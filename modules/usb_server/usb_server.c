#include "usb_server.h"
#include <string.h>
#include <driver/usb_serial_jtag.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "pubsub.h"
#include "messages.h"
#include "platform.h"

#define TAG "usb_server"

static TaskHandle_t g_rx_task_handle = NULL;

// ---------------------------------------------------------------------------
// UDP/UART → USB: forward received bytes to USB host
// ---------------------------------------------------------------------------
static void on_packet_to_usb(uint8_t *data, size_t size) {
    if (size < sizeof(raw_packet_t)) return;
    raw_packet_t *pkt = (raw_packet_t *)data;
    if (!pkt->data || pkt->len == 0) return;

    usb_serial_jtag_write_bytes((const char *)pkt->data, pkt->len,
                                20 / portTICK_PERIOD_MS);
}

// ---------------------------------------------------------------------------
// USB RX: forward raw bytes, publish USB_RECEIVED
// ---------------------------------------------------------------------------
static void usb_rx_task(void *arg) {
    uint8_t rx_buf[256];

    while (1) {
        int len = usb_serial_jtag_read_bytes(rx_buf, sizeof(rx_buf),
                                             20 / portTICK_PERIOD_MS);
        if (len <= 0) continue;

        led_recv();
        raw_packet_t pkt = { .data = rx_buf, .len = (size_t)len };
        publish(USB_RECEIVED, (uint8_t *)&pkt, sizeof(raw_packet_t));
    }
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void usb_server_setup(void) {
    usb_serial_jtag_driver_config_t usb_cfg = {
        .rx_buffer_size = 1024,
        .tx_buffer_size = 1024,
    };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_cfg));

    subscribe(UDP_RECEIVED, on_packet_to_usb);

    xTaskCreate(usb_rx_task, "usb_rx", 4096, NULL, 10, &g_rx_task_handle);

    ESP_LOGI(TAG, "USB-CDC serial");
}
