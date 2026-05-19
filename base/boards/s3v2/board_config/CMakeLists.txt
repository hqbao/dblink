idf_component_register(SRCS "platform_led.c"
                       INCLUDE_DIRS "."
                       REQUIRES led_strip esp_timer)

# Allow build-time overrides via:
#   idf.py -DFC_BAUD_RATE=230400 build
#   idf.py -DENABLE_WIFI_AP=1     build
foreach(_def FC_BAUD_RATE ENABLE_WIFI_AP)
    if(DEFINED ${_def})
        target_compile_definitions(${COMPONENT_LIB} INTERFACE ${_def}=${${_def}})
    endif()
endforeach()
