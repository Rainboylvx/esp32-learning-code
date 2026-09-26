# 将 main.c 注册为 main 组件的源文件。
# PRIV_REQUIRES 表示只有本组件内部需要 GPIO 驱动，
# 其他组件不会因为依赖 main 而自动获得这个依赖。
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_driver_gpio)
