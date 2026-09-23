#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"
#include <esp_system.h>
#include "esp_mac.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_random.h"

/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "gatt_svr.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#if CONFIG_LED_SETUP
#include "led.h"
#endif
#ifdef CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif

#define TAG "CAR_BLE"

static uint8_t mac[6];

typedef enum
{
  CAR_STATE_STOP,
  CAR_STATE_RUN_UP,
  CAR_STATE_RUN_DOWN,
  CAR_STATE_RUN_LEFT,
  CAR_STATE_RUN_RIGHT
} car_state_t;

typedef enum
{
  CAR_MODE_REMOTE_CONTROL,
  CAR_MODE_STUDY,
  CAR_MODE_NAV,
  CAR_MODE_AUTO,
  CAR_MODE_BACK_SAME_WAY,
} car_mode_t;

typedef struct
{
  car_state_t state;
  double time;
} car_nav_t;

typedef struct
{
  car_nav_t *items;
  size_t size;
  size_t capacity;
} CarNavList;

CarNavList *history = NULL;

car_state_t car_state = CAR_STATE_STOP;
car_mode_t car_mode = CAR_MODE_REMOTE_CONTROL;

static int64_t car_start_time = 0;

#define CAR_TURN_AROUND 400 // 小车掉头时间参数(实验程序，按照时间控制，根据电压情况和电机实际转动情况进行时间调整).

CarNavList *car_nav_list_create(size_t initial_capacity);
void car_nav_list_destroy(CarNavList *list);
int car_nav_list_append(CarNavList *list, car_nav_t item);
car_nav_t *car_nav_list_get(const CarNavList *list, size_t index);
size_t car_nav_list_size(const CarNavList *list);
void car_nav_list_clear(CarNavList *list);
int car_nav_list_insert(CarNavList *list, size_t index, car_nav_t item);
int car_nav_list_remove(CarNavList *list, size_t index);

void car_up(void);
void car_down(void);
void car_left(void);
void car_right(void);
void car_stop(bool save);

// 创建新列表
CarNavList *car_nav_list_create(size_t initial_capacity)
{
  CarNavList *list = (CarNavList *)malloc(sizeof(CarNavList));
  if (!list)
    return NULL;

  list->capacity = initial_capacity > 0 ? initial_capacity : 10;
  list->items = (car_nav_t *)malloc(list->capacity * sizeof(car_nav_t));
  if (!list->items)
  {
    free(list);
    return NULL;
  }

  list->size = 0;
  return list;
}

// 销毁列表
void car_nav_list_destroy(CarNavList *list)
{
  if (list)
  {
    free(list->items);
    free(list);
  }
}

// 扩容（内部函数）
static int car_nav_list_resize(CarNavList *list, size_t new_capacity)
{
  car_nav_t *new_items = (car_nav_t *)realloc(list->items, new_capacity * sizeof(car_nav_t));
  if (!new_items)
    return 0; // 扩容失败

  list->items = new_items;
  list->capacity = new_capacity;
  return 1;
}

// 添加元素到末尾
int car_nav_list_append(CarNavList *list, car_nav_t item)
{
  if (list->size >= list->capacity)
  {
    // 容量不足时自动扩容（通常扩容1.5倍或2倍）
    size_t new_capacity = list->capacity * 2;
    if (!car_nav_list_resize(list, new_capacity))
    {
      return 0; // 扩容失败
    }
  }

  list->items[list->size] = item;
  list->size++;
  return 1; // 成功
}

// 获取指定位置的元素指针（允许修改）
car_nav_t *car_nav_list_get(const CarNavList *list, size_t index)
{
  if (index >= list->size)
    return NULL;
  return &list->items[index];
}

// 获取列表当前大小
size_t car_nav_list_size(const CarNavList *list)
{
  return list->size;
}

// 清空列表（不释放内存）
void car_nav_list_clear(CarNavList *list)
{
  list->size = 0;
}

// 在指定位置插入元素
int car_nav_list_insert(CarNavList *list, size_t index, car_nav_t item)
{
  if (index > list->size)
    return 0; // 索引越界

  // 检查容量
  if (list->size >= list->capacity)
  {
    size_t new_capacity = list->capacity * 2;
    if (!car_nav_list_resize(list, new_capacity))
    {
      return 0;
    }
  }

  // 将index及之后的元素后移
  memmove(&list->items[index + 1], &list->items[index],
          (list->size - index) * sizeof(car_nav_t));

  list->items[index] = item;
  list->size++;
  return 1;
}

// 删除指定位置的元素
int car_nav_list_remove(CarNavList *list, size_t index)
{
  if (index >= list->size)
    return 0;

  // 将index之后的元素前移
  memmove(&list->items[index], &list->items[index + 1],
          (list->size - index - 1) * sizeof(car_nav_t));

  list->size--;
  return 1;
}

SemaphoreHandle_t g_car_trigger_sem = NULL;

static car_state_t get_random_state(void)
{
  int i = (esp_random() % 4) + 1;
  if (i == 3)
  {
    return CAR_STATE_RUN_LEFT;
  }
  else if (i == 4)
  {
    return CAR_STATE_RUN_RIGHT;
  }
  else
  {
    return CAR_STATE_RUN_UP;
  }
}

// 实验程序，按照时间控制，根据电压情况和电机实际转动情况进行时间调整
static void car_task(void *ctx)
{
  while (1)
  {
    if (xSemaphoreTake(g_car_trigger_sem, portMAX_DELAY) == pdTRUE)
    {
      if (car_mode == CAR_MODE_AUTO)
      {
        while (car_mode == CAR_MODE_AUTO)
        {
          int time = ((esp_random() % 3) + 1) * 1000;

          car_state = get_random_state();
          if (car_state == CAR_STATE_RUN_LEFT)
          {
            time = ((esp_random() % 3) + 1) * 100;
            car_left();
          }
          else if (car_state == CAR_STATE_RUN_RIGHT)
          {
            time = ((esp_random() % 3) + 1) * 100;
            car_right();
          }
          else
          {
            car_up();
          }

          vTaskDelay(pdMS_TO_TICKS(time));
        }
        car_stop(false);
        car_mode = CAR_MODE_REMOTE_CONTROL;
      }
      else if (car_mode == CAR_MODE_NAV)
      {
        if (history != NULL)
        {
          for (size_t i = 0; i < car_nav_list_size(history); i++)
          {
            car_nav_t *record = car_nav_list_get(history, i);
            if (record)
            {
              if (record->state == CAR_STATE_RUN_UP)
              {
                car_up();
              }
              else if (record->state == CAR_STATE_RUN_DOWN)
              {
                car_down();
              }
              else if (record->state == CAR_STATE_RUN_LEFT)
              {
                car_left();
              }
              else if (record->state == CAR_STATE_RUN_RIGHT)
              {
                car_right();
              }
              else
              {
                continue;
              }
              vTaskDelay(pdMS_TO_TICKS(record->time));
            }
          }
          car_stop(false);
        }
        else
        {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
          ESP_LOGI(TAG, "history != NULL");
#endif
        }
        car_mode = CAR_MODE_REMOTE_CONTROL;
      }
      else if (car_mode == CAR_MODE_BACK_SAME_WAY)
      {
        if (history != NULL)
        {
          car_left();
          vTaskDelay(pdMS_TO_TICKS(CAR_TURN_AROUND));
          car_stop(false);

          for (size_t i = car_nav_list_size(history); i-- > 0;)
          {
            car_nav_t *record = car_nav_list_get(history, i);
            if (record)
            {
              if (record->state == CAR_STATE_RUN_UP)
              {
                car_up();
              }
              else if (record->state == CAR_STATE_RUN_DOWN)
              {
                car_down();
              }
              else if (record->state == CAR_STATE_RUN_LEFT)
              {
                car_right();
              }
              else if (record->state == CAR_STATE_RUN_RIGHT)
              {
                car_left();
              }
              else
              {
                continue;
              }
              vTaskDelay(pdMS_TO_TICKS(record->time));
            }
          }
          car_stop(false);
        }
        else
        {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
          ESP_LOGI(TAG, "history != NULL");
#endif
        }
        car_mode = CAR_MODE_REMOTE_CONTROL;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }

  vTaskDelete(NULL);
}

// 停止所有电机
void car_stop(bool save)
{
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "car stop");
#endif
  gpio_set_level(CONFIG_MOTOR_A_IN1, 0);
  gpio_set_level(CONFIG_MOTOR_A_IN2, 0);
  gpio_set_level(CONFIG_MOTOR_B_IN3, 0);
  gpio_set_level(CONFIG_MOTOR_B_IN4, 0);

  if (save && car_mode == CAR_MODE_STUDY && car_state != CAR_STATE_STOP)
  {
    if (car_start_time > 0 && history != NULL)
    {
      int64_t end_time = esp_timer_get_time();

      double elapsed_ms = (end_time - car_start_time) / 1000.0;
      car_start_time = 0;

      // 记录方向+时间
      if (elapsed_ms > 0)
      {
        car_nav_t _nav = {.state = car_state, .time = elapsed_ms};

        car_nav_list_append(history, _nav);

#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
        ESP_LOGI(TAG, "car_nav_list_append: %d %d", (int)elapsed_ms, car_state);
#endif
      }
    }
  }

  car_state = CAR_STATE_STOP;

#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "car stop end.");
#endif
}

// 前进
void car_up(void)
{
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "前");
#endif
  car_stop(false);
  car_state = CAR_STATE_RUN_UP;

  if (car_mode == CAR_MODE_STUDY)
  {
    car_start_time = esp_timer_get_time();
  }

  // 电机A正转
  gpio_set_level(CONFIG_MOTOR_A_IN1, 1);
  gpio_set_level(CONFIG_MOTOR_A_IN2, 0);
  vTaskDelay(pdMS_TO_TICKS(50));
  // 电机B正转
  gpio_set_level(CONFIG_MOTOR_B_IN3, 1);
  gpio_set_level(CONFIG_MOTOR_B_IN4, 0);
}

// 后退
void car_down(void)
{
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "后");
#endif
  car_stop(false);
  car_state = CAR_STATE_RUN_DOWN;

  if (car_mode == CAR_MODE_STUDY)
  {
    car_start_time = esp_timer_get_time();
  }

  // 电机A反转
  gpio_set_level(CONFIG_MOTOR_A_IN1, 0);
  gpio_set_level(CONFIG_MOTOR_A_IN2, 1);
  vTaskDelay(pdMS_TO_TICKS(50));
  // 电机B反转
  gpio_set_level(CONFIG_MOTOR_B_IN3, 0);
  gpio_set_level(CONFIG_MOTOR_B_IN4, 1);
}

// 左转
void car_left(void)
{
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "左");
#endif
  car_stop(false);
  car_state = CAR_STATE_RUN_LEFT;

  if (car_mode == CAR_MODE_STUDY)
  {
    car_start_time = esp_timer_get_time();
  }

  gpio_set_level(CONFIG_MOTOR_A_IN1, 0);
  gpio_set_level(CONFIG_MOTOR_A_IN2, 1);
  vTaskDelay(pdMS_TO_TICKS(50));
  gpio_set_level(CONFIG_MOTOR_B_IN3, 1);
  gpio_set_level(CONFIG_MOTOR_B_IN4, 0);
}

// 右转
void car_right(void)
{
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "右");
#endif
  car_stop(false);
  car_state = CAR_STATE_RUN_RIGHT;

  if (car_mode == CAR_MODE_STUDY)
  {
    car_start_time = esp_timer_get_time();
  }

  gpio_set_level(CONFIG_MOTOR_A_IN1, 1);
  gpio_set_level(CONFIG_MOTOR_A_IN2, 0);
  vTaskDelay(pdMS_TO_TICKS(50));
  gpio_set_level(CONFIG_MOTOR_B_IN3, 0);
  gpio_set_level(CONFIG_MOTOR_B_IN4, 1);
}

// 初始化小车引脚
static void car_init(void)
{
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "car init %d,%d,%d,%d", CONFIG_MOTOR_A_IN1, CONFIG_MOTOR_A_IN2, CONFIG_MOTOR_B_IN3, CONFIG_MOTOR_B_IN4);
#endif

  // 配置GPIO为输出模式
  gpio_config_t io_conf = {
      .pin_bit_mask = 0,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};

  // 为每个引脚设置位掩码
  io_conf.pin_bit_mask |= (1ULL << CONFIG_MOTOR_A_IN1);
  io_conf.pin_bit_mask |= (1ULL << CONFIG_MOTOR_A_IN2);
  io_conf.pin_bit_mask |= (1ULL << CONFIG_MOTOR_B_IN3);
  io_conf.pin_bit_mask |= (1ULL << CONFIG_MOTOR_B_IN4);

  gpio_config(&io_conf);

  // 初始状态：所有电机停止
  car_stop(false);
}

#if CONFIG_KEY_SETUP

#define DEBOUNCE_TIME_MS 20 // 消抖

static TimerHandle_t long_press_timer = NULL;
static QueueHandle_t key_event_queue = NULL;

// 按键事件类型
typedef enum
{
  KEY_EVENT_PRESS,
  KEY_EVENT_RELEASE
} key_event_t;

// 消抖任务函数
static void key_debounce_task(void *arg)
{
  key_event_t event;
  uint32_t last_press_time = 0;

  while (1)
  {
    if (xQueueReceive(key_event_queue, &event, portMAX_DELAY))
    {
      uint32_t now = xTaskGetTickCount();

      if ((now - last_press_time) * portTICK_PERIOD_MS >= DEBOUNCE_TIME_MS)
      {
        last_press_time = now;

        if (event == KEY_EVENT_PRESS)
        {
          if (xTimerIsTimerActive(long_press_timer) == pdFALSE)
          {
            xTimerStart(long_press_timer, 0);
          }
          else
          {
            xTimerReset(long_press_timer, 0);
          }
        }
        else
        {
          if (xTimerStop(long_press_timer, 0) == pdPASS)
          {
            // 这里可以处理短按逻辑
          }
        }
      }
    }
  }
}

// 定义一个信号量，用于定时器回调和业务任务间的通信
SemaphoreHandle_t g_timer_trigger_sem = NULL;

static void key_business_task(void *ctx)
{
  while (1)
  {
    if (xSemaphoreTake(g_timer_trigger_sem, portMAX_DELAY) == pdTRUE)
    {
      // 长按任务处理
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }

  vTaskDelete(NULL);
}

// 长按定时器回调
static void long_press_timer_callback(TimerHandle_t xTimer)
{
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "检测到长按3秒！");
#endif

  xSemaphoreGive(g_timer_trigger_sem);
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
  static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  key_event_t event;

  int level = gpio_get_level(CONFIG_KEY_PIN);
  event = (level == 0) ? KEY_EVENT_PRESS : KEY_EVENT_RELEASE;

  xQueueSendFromISR(key_event_queue, &event, &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}

void key_init(void)
{
  // 创建一个二进制信号量，用于同步（初始值为0）
  g_timer_trigger_sem = xSemaphoreCreateBinary();

  gpio_config_t gpio_conf = {
      .pin_bit_mask = (1ULL << CONFIG_KEY_PIN),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE,
  };
  ESP_ERROR_CHECK(gpio_config(&gpio_conf));

  key_event_queue = xQueueCreate(10, sizeof(key_event_t));
  if (key_event_queue == NULL)
  {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
    ESP_LOGE(TAG, "队列创建失败！");
#endif
    return;
  }

  long_press_timer = xTimerCreate(
      "LongPressTimer",
      pdMS_TO_TICKS(CONFIG_LONG_PRESS_TIME * 1000),
      pdFALSE,
      NULL,
      long_press_timer_callback);

  if (long_press_timer == NULL)
  {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
    ESP_LOGE(TAG, "定时器创建失败！");
#endif
    vQueueDelete(key_event_queue);
    return;
  }

  xTaskCreate(key_debounce_task, "key_debounce", 2048, NULL, 5, NULL);

  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_KEY_PIN, gpio_isr_handler, NULL));

  // 创建业务任务
  xTaskCreate(key_business_task, "key_business_task", 4096, NULL, 4, NULL);

#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "按键初始化完成");
#endif
}

void key_deinit(void)
{
  if (long_press_timer)
  {
    xTimerDelete(long_press_timer, portMAX_DELAY);
  }
  if (key_event_queue)
  {
    vQueueDelete(key_event_queue);
  }
  gpio_isr_handler_remove(CONFIG_KEY_PIN);
}
#endif

void host_task(void *param)
{
  nimble_port_run();
  nimble_port_freertos_deinit();
}

void ble_data_callback(uint8_t *data, uint16_t len)
{
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "主程序收到蓝牙数据, 长度=%d", len);
#endif

  if (len >= 3 && data[0] == 0xbf && data[len - 1] == 0xef)
  {
    switch (data[1])
    {
    case 0x00:
      car_stop(true);
      break;
    case 0x01:
      car_up();
      break;
    case 0x02:
      car_down();
      break;
    case 0x03:
      car_left();
      break;
    case 0x04:
      car_right();
      break;
    case 0x05:
      car_mode = CAR_MODE_STUDY;
      if (history == NULL)
      {
        car_nav_list_destroy(history);
        history = NULL;
      }

      history = car_nav_list_create(20);
      if (!history)
      {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
        ESP_LOGE(TAG, "列表创建失败！");
#endif
      }
      break;
    case 0x06:
      car_mode = CAR_MODE_BACK_SAME_WAY;
      xSemaphoreGive(g_car_trigger_sem);
      break;
    case 0x07:
      car_mode = CAR_MODE_NAV;
      xSemaphoreGive(g_car_trigger_sem);
      break;
    case 0x08:
      car_mode = CAR_MODE_REMOTE_CONTROL;
      break;
    case 0x09:
      car_mode = CAR_MODE_AUTO;
      xSemaphoreGive(g_car_trigger_sem);
      break;
    case 0x0a:
      car_mode = CAR_MODE_REMOTE_CONTROL;
      break;
    case 0x0b:
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
      ESP_LOGE(TAG, "语音数据");
#endif
      break;

    default:
      break;
    }
  }
}

void app_main(void)
{
  ESP_ERROR_CHECK(nvs_flash_init());

#ifdef CONFIG_PM_ENABLE
  esp_pm_config_t pm_config = {
      .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
      .min_freq_mhz = 80,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
      .light_sleep_enable = true
#endif
  };
  esp_pm_configure(&pm_config);
#endif

  esp_err_t ret = esp_read_mac(mac, ESP_MAC_BT);
  if (ret != ESP_OK)
  {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
    ESP_LOGE(TAG, "读取芯片MAC失败: %s", esp_err_to_name(ret));
#endif
    return;
  }

#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "ble mac: %02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif

  // 初始化小车引脚
  car_init();

// LED引脚初始化
#if CONFIG_LED_SETUP
#if CONFIG_BOARD_TYPE_S3
  led_init();
  set_led_cyan();
#endif
#if CONFIG_BOARD_TYPE_C3
  led_init();
  set_led_left(1);
  set_led_right(1);
  vTaskDelay(pdMS_TO_TICKS(350));
  set_led_left(0);
  set_led_right(0);
#endif
#endif

// 按键引脚初始化
#if CONFIG_KEY_SETUP
  key_init();
#endif

  // BLE初始化
  ESP_ERROR_CHECK(nimble_port_init());

  ble_hs_cfg.sync_cb = ble_app_on_sync;

  /* Initialize the GATT server */
  int rc = gatt_svr_init();
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
  ESP_LOGI(TAG, "gatt_svr_init rc=%d", rc);
#endif

  ble_register_data_callback(ble_data_callback);

  nimble_port_freertos_init(host_task);

  g_car_trigger_sem = xSemaphoreCreateBinary();

  xTaskCreate(car_task, "car_task", 4096, NULL, 8, NULL);
}