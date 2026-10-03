#include <stdint.h>
#include "esp_attr.h"
#include "esp_err.h"
#include "exit.h"

// 中断服务程序与任务之间传递事件的队列句柄，由 exit_init() 注入。
static QueueHandle_t s_event_queue;

// GPIO 中断服务程序（ISR）：只做“记录事件”这一件最轻量的事。
// 必须放在 IRAM 中（IRAM_ATTR），保证 Flash 被禁用时仍能执行。
static void IRAM_ATTR boot_gpio_isr(void *arg)
{
    // 注册中断时把引脚号作为参数传入，这里还原出来。
    const uint32_t gpio_num = (uint32_t)(uintptr_t)arg;
    // 标记本次退出 ISR 后是否需要立即切换任务。
    BaseType_t higher_priority_task_woken = pdFALSE;

    // 从中断上下文发送到队列，绝不阻塞；若唤醒的任务优先级更高，
    // higher_priority_task_woken 会被置为 pdTRUE。
    xQueueSendFromISR(s_event_queue, &gpio_num, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE) {
        // 请求立即进行任务切换，避免高优先级任务等待到下一个 tick。
        portYIELD_FROM_ISR();
    }
}

// 初始化 BOOT 按键中断：配置 GPIO、安装 ISR 服务并绑定处理函数。
// event_queue 需由调用方创建，中断触发时会向其中投递引脚号。
esp_err_t exit_init(QueueHandle_t event_queue)
{
    if (event_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_event_queue = event_queue;

    // GPIO 配置：BOOT 按键接在 GPIO0，按下接地，因此使用上拉 + 下降沿触发。
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << BOOT_INT_GPIO_PIN,// num_0
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    // 安装全局 ISR 服务；ESP_INTR_FLAG_IRAM 允许中断在 Flash 不可用时运行。
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (err != ESP_OK) {
        return err;
    }
    // 为指定引脚挂载中断处理函数，并把引脚号作为参数传给 ISR。
    err = gpio_isr_handler_add(BOOT_INT_GPIO_PIN,
                               boot_gpio_isr,
                               (void *)(uintptr_t)BOOT_INT_GPIO_PIN);
    if (err != ESP_OK) {
        return err;
    }
    // 最后使能该引脚的中断。
    return gpio_intr_enable(BOOT_INT_GPIO_PIN);// dis
}

// 返回内部使用的事件队列句柄，供任务侧接收按键事件。
QueueHandle_t exit_event_queue(void)
{
    return s_event_queue;
}
