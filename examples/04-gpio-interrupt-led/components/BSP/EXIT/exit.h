#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define BOOT_INT_GPIO_PIN GPIO_NUM_0

esp_err_t exit_init(QueueHandle_t event_queue);
QueueHandle_t exit_event_queue(void);
