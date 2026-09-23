#ifndef __LED_H__
#define __LED_H__

#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_1

void led_init(void);
void led_task(void *pvParameters);

#endif /* __LED_H__ */
