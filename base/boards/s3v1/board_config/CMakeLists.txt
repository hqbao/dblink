idf_component_register(SRCS "platform_led.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_timer)

# Allow build-time overrides via: idf.py -DFC_BAUD_RATE=230400 build
foreach(_def FC_BAUD_RATE)
    if(DEFINED ${_def})
        target_compile_definitions(${COMPONENT_LIB} INTERFACE ${_def}=${${_def}})
    endif()
endforeach()
