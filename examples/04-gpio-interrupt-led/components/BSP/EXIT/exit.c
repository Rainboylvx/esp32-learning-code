#include <stdint.h>
#include "esp_attr.h"
#include "esp_err.h"
#include "exit.h"

static QueueHandle_t s_event_queue;

static void IRAM_ATTR boot_gpio_isr(void *arg)
{
    const uint32_t gpio_num = (uint32_t)(uintptr_t)arg;
    BaseType_t higher_priority_task_woken = pdFALSE;

    xQueueSendFromISR(s_event_queue, &gpio_num, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

esp_err_t exit_init(QueueHandle_t event_queue)
{
    if (event_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_event_queue = event_queue;

    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << BOOT_INT_GPIO_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_isr_handler_add(BOOT_INT_GPIO_PIN,
                               boot_gpio_isr,
                               (void *)(uintptr_t)BOOT_INT_GPIO_PIN);
    if (err != ESP_OK) {
        return err;
    }
    return gpio_intr_enable(BOOT_INT_GPIO_PIN);
}

QueueHandle_t exit_event_queue(void)
{
    return s_event_queue;
}
