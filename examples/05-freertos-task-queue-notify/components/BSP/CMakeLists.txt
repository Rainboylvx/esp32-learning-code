# BSP 同时包含 LED 输出和 KEY 输入两个模块。
# 两个模块都使用 GPIO 驱动，KEY 的消抖还会调用 FreeRTOS 延时。
idf_component_register(SRCS "LED/led.c" "KEY/key.c"
                    INCLUDE_DIRS "LED" "KEY"
                    PRIV_REQUIRES esp_driver_gpio freertos)
