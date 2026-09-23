#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"


/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    led_init();             /* 初始化LED */

    while(1)
    {
        LED_TOGGLE();
        vTaskDelay(500);   /* 延时500ms */
    }
}
