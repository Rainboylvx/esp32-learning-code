# BSP 只封装 DNESP32S3 板载 LED 的 GPIO 细节。
idf_component_register(SRCS "LED/led.c"
                    INCLUDE_DIRS "LED"
                    PRIV_REQUIRES esp_driver_gpio)
