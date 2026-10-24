idf_component_register(SRCS "KEY/key.c" "PWM_LED/pwm_led.c" "PWM_LED/fade.c"
    INCLUDE_DIRS "KEY" "PWM_LED" PRIV_REQUIRES esp_driver_gpio esp_driver_ledc freertos)
