idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES BSP freertos esp_driver_gpio)

# BSP: components/BSP/CMakeLists.txt
# 
