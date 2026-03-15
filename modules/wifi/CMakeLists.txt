idf_component_register(SRCS "wifi.c"
                       INCLUDE_DIRS "."
                       REQUIRES foundation board_config esp_wifi esp_event esp_netif)
