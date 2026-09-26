# 把 LED/led.c 注册为 BSP 组件的实现文件。
# INCLUDE_DIRS 会公开 LED 目录，使 main 组件可以直接包含 led.h。
# GPIO 驱动只在 BSP 内部使用，因此声明为私有依赖。
idf_component_register(SRCS "LED/led.c"
                    INCLUDE_DIRS "LED"
                    PRIV_REQUIRES esp_driver_gpio)
