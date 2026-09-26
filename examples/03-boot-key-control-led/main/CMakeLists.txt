# main 只负责组合 KEY 和 LED 两个 BSP 接口。
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES BSP)
