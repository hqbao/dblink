#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_camera.h>
#include <driver/uart.h>
#include <driver/i2c.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <string.h>
#include "server.h"

#define TAG "main.c"

#define SSID "dbcam"
#define PASS ""

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

#define RC_FREQ           20

typedef struct {
  uint8_t byte;
  uint8_t buffer[128];
  uint8_t header[2];
  char stage;
  uint16_t payload_size;
  int buffer_idx;
} uart_rx_t;

void delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
int64_t get_time(void) { return esp_timer_get_time(); }
uint32_t millis(void) { return (uint32_t)(esp_timer_get_time()/1000); }
void flash(uint8_t count) {}

TaskHandle_t task_hangle_1 = NULL;
TaskHandle_t task_hangle_2 = NULL;

volatile char g_frame_captured = 0;

rc_t g_rc;
rc_t g_rc_prev;

int g_roll = 0;
int g_pitch = 0;
int g_yaw = 0;

void core0();
void core1();

static void init_cam(void);
static void init_wifi_ap(void);

void app_main(void) {
  ESP_LOGI(TAG, "Start program");

  xTaskCreatePinnedToCore(core0, "Core 0 loop", 4096, NULL, 2, &task_hangle_1, 0);
  xTaskCreatePinnedToCore(core1, "Core 1 loop", 4096, NULL, 3, &task_hangle_2, 1);

  while (1) {delay(1000);}
}

void core0() {
  init_cam();
  init_wifi_ap();

  while (1) {delay(1000);}
}

static void fc_init(void) {
  const uart_config_t uart_config = {
    .baud_rate = 19200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };
  ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 43, 44, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void rc_timer(void *param) {
  rc_get(&g_rc);
}

static void handle_db_msg(uart_rx_t *msg) {
  if (msg->header[0] == 'b' && msg->header[1] == 'd') { // DB message
    if (msg->buffer[0] == 0x00 && msg->buffer[1] == 0x00) { // Euler Angle
      g_roll = *(int*)&msg->buffer[4];
      g_pitch = *(int*)&msg->buffer[8];
      g_yaw = *(int*)&msg->buffer[12];
      // ESP_LOGI(TAG, "Roll: %d\tPitch: %d\tYaw: %d\t", g_roll/1000, g_pitch/1000, g_yaw/1000);
    }
  }
}

void core1() {
  // Init timer
  fc_init();

  // Start tranceivering RC values
  const esp_timer_create_args_t timer_args1 = {
    .callback = &rc_timer,
    .name = "RC timer"
  };
  esp_timer_handle_t timer_handler1;
  ESP_ERROR_CHECK(esp_timer_create(&timer_args1, &timer_handler1));
  ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handler1, 1000000/RC_FREQ));

  uart_rx_t g_uart_rx1 = {0, {0}, {0}, 0, 0, 0};
  while (1) {
    int len = uart_read_bytes(UART_NUM_1, &g_uart_rx1.byte, 1, 20 / portTICK_PERIOD_MS);
    if (len != 1) continue;

    if (g_uart_rx1.stage == 5) {
      g_uart_rx1.buffer[g_uart_rx1.buffer_idx] = g_uart_rx1.byte;
      g_uart_rx1.buffer_idx++;
      // Plus 2-byte class-id, 2-byte length and 2-byte checksum
      if (g_uart_rx1.buffer_idx == g_uart_rx1.payload_size + 6) {
        g_uart_rx1.stage = 0;
        handle_db_msg(&g_uart_rx1);
      }
    }
    else if (g_uart_rx1.stage == 0) {
      if (g_uart_rx1.byte == 'b') {
        g_uart_rx1.header[0] = g_uart_rx1.byte;
        g_uart_rx1.stage = 1;
      }
    }
    else if (g_uart_rx1.stage == 1) {
      if (g_uart_rx1.byte == 'd') {
        g_uart_rx1.header[1] = g_uart_rx1.byte;
        g_uart_rx1.buffer_idx = 0;
        g_uart_rx1.stage = 2;
      }
      else g_uart_rx1.stage = 0;
    }
    else if (g_uart_rx1.stage == 2) {
      g_uart_rx1.buffer[g_uart_rx1.buffer_idx] = g_uart_rx1.byte;
      g_uart_rx1.buffer_idx = 1;
      g_uart_rx1.stage = 3;
    }
    else if (g_uart_rx1.stage == 3) {
      g_uart_rx1.buffer[g_uart_rx1.buffer_idx] = g_uart_rx1.byte;
      g_uart_rx1.buffer_idx = 2;
      g_uart_rx1.stage = 4;
    }
    else if (g_uart_rx1.stage == 4) {
      g_uart_rx1.buffer[g_uart_rx1.buffer_idx] = g_uart_rx1.byte;
      g_uart_rx1.buffer_idx++;
      if (g_uart_rx1.buffer_idx == 4) {
        g_uart_rx1.payload_size = *(uint16_t*)&g_uart_rx1.buffer[2];
        g_uart_rx1.stage = 5;
      }
    }
    else {
      g_uart_rx1.stage = 0;
    }
  }
}

static void init_cam(void) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  ESP_ERROR_CHECK(esp_camera_init(&config));
}

static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base,
  int32_t event_id, void* event_data) {
  ESP_LOGI(TAG, "wifi_event_handler: event_id = %ld", event_id);
  if (event_id == WIFI_EVENT_AP_START) {
    server_start();
  }
  else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    ESP_LOGI(TAG, "Connected AID=%d", event->aid);
  }
  else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    ESP_LOGI(TAG, "Disconnected AID=%d", event->aid);
  }
}

static void init_wifi_ap(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
    .ap = {
      .ssid = SSID,
      .ssid_len = strlen(SSID),
      .channel = 0,
      .password = PASS,
      .max_connection = 10,
      .authmode = WIFI_AUTH_WPA3_PSK,
      .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
      .pmf_cfg = {
        .required = true,
      },
    },
  };

  if (strlen(PASS) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Finished. ssid:%s, password:%s", SSID, PASS);
}
