# main 组合 KEY、LED 与 FreeRTOS 通信；GPIO 细节仍封装在 BSP 中。
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES BSP freertos)
