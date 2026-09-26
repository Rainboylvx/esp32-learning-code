# main 组件只调用 BSP 提供的 LED 接口，不再直接依赖 GPIO 驱动。
# 这样应用层无需知道 LED 接在哪个引脚、以什么电平点亮。
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES BSP)
