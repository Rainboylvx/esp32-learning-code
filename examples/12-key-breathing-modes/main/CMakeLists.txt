idf_component_register(SRCS "main.c" "app_model.c" INCLUDE_DIRS "."
    PRIV_REQUIRES BSP esp_timer freertos)
