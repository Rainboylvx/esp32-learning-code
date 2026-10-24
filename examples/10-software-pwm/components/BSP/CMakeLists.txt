idf_component_register(SRCS "SOFT_PWM/soft_pwm.c" INCLUDE_DIRS "SOFT_PWM"
    PRIV_REQUIRES esp_driver_gpio esp_driver_gptimer freertos)
