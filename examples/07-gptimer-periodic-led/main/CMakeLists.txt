# main 依赖自己的 BSP；具体 GPIO 和定时器驱动由 BSP 声明。
idf_component_register(SRCS "main.c" INCLUDE_DIRS "."
    PRIV_REQUIRES BSP esp_timer freertos)
