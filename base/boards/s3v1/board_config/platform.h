#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <inttypes.h>

// === WiFi Configuration ===
#define ENABLE_WIFI_AP    0

#define WIFI_STA_SSID     "SkyDrone"
#define WIFI_STA_PASS     "12345678"

#define WIFI_AP_SSID      "SkyDrone"
#define WIFI_AP_PASS      "12345678"   // min 8 chars, or "" for open
#define WIFI_AP_CHANNEL   1
#define WIFI_AP_MAX_CONN  2

// === Debug ===
#define ENABLE_DEBUG_LOGGING 0

#define platform_console(fmt, ...) printf(fmt, ##__VA_ARGS__)

// === Serial Interface ===
// 0 = UART1 on GPIO 43/44 (connect to flight controller)
// 1 = USB-CDC (test via USB cable with test_uart_bridge.py)
#define UART_USE_USB      1

#define UART_TX_PIN       43
#define UART_RX_PIN       44

#endif
