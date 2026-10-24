idf_component_register(SRCS "KEY/key.c" "LED/led.c"
    INCLUDE_DIRS "KEY" "LED" PRIV_REQUIRES esp_driver_gpio)
