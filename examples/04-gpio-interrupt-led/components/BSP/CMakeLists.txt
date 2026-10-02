idf_component_register(SRCS "LED/led.c" "EXIT/exit.c"
                    INCLUDE_DIRS "LED" "EXIT"
                    PRIV_REQUIRES esp_driver_gpio freertos)
