idf_component_register(SRCS "LED/led.c" "TICK/tick.c"
    INCLUDE_DIRS "LED" "TICK"
    PRIV_REQUIRES esp_driver_gpio esp_driver_gptimer freertos)
