#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <inttypes.h>

// === WiFi Configuration ===
// ENABLE_WIFI_AP defaults to 0 (STA mode). Override at build time with:
//   idf.py -DENABLE_WIFI_AP=1 build
#ifndef ENABLE_WIFI_AP
#define ENABLE_WIFI_AP    0
#endif

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
#define UART_TX_PIN       43
#define UART_RX_PIN       44
#ifndef FC_BAUD_RATE
#define FC_BAUD_RATE      115200
#endif

// === LED ===
#define LED_PIN           21   // Xiao ESP32-S3 built-in LED (active-low GPIO)

void led_init(void);
void led_not_connected(void);  // LED on — no WiFi
void led_connecting(void);     // LED on — STA connecting
void led_connected(void);      // LED off — WiFi ready
void led_send(void);           // LED flash — sending data
void led_recv(void);           // LED flash — receiving data
void led_off(void);

#endif
