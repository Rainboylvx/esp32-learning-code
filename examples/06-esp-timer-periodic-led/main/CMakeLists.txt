# main 使用 BSP、FreeRTOS 任务通知和 ESP Timer。
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES BSP freertos esp_timer)
