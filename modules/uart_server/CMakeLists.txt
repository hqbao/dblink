idf_component_register(SRCS "uart_server.c"
                       INCLUDE_DIRS "."
                       REQUIRES foundation board_config driver freertos)
