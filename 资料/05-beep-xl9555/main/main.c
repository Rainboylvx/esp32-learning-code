#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "beep_xl9555"

#define BOARD_I2C_PORT            I2C_NUM_0
#define BOARD_I2C_SDA_IO          GPIO_NUM_41
#define BOARD_I2C_SCL_IO          GPIO_NUM_42
#define BOARD_I2C_FREQ_HZ         400000

#define XL9555_I2C_ADDR           0x20
#define XL9555_INPUT_PORT0_REG    0x00
#define XL9555_OUTPUT_PORT0_REG   0x02
#define XL9555_CONFIG_PORT0_REG   0x06

#define XL9555_CONFIG_DEFAULT     0xF003
#define XL9555_BEEP_IO            0x0008
#define XL9555_SPK_EN_IO          0x0004

#define BEEP_ON_TIME_MS           120
#define BEEP_OFF_TIME_MS          1200

static i2c_master_bus_handle_t s_i2c_bus;
static i2c_master_dev_handle_t s_xl9555_dev;

static esp_err_t xl9555_write_reg(uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t payload[3];

    ESP_RETURN_ON_FALSE(len <= 2, ESP_ERR_INVALID_ARG, TAG, "write len too large");

    payload[0] = reg;
    for (size_t i = 0; i < len; ++i) {
        payload[i + 1] = data[i];
    }

    return i2c_master_transmit(s_xl9555_dev, payload, len + 1, -1);
}

static esp_err_t xl9555_read_ports(uint8_t data[2])
{
    uint8_t reg = XL9555_INPUT_PORT0_REG;
    return i2c_master_transmit_receive(s_xl9555_dev, &reg, 1, data, 2, -1);
}

static esp_err_t xl9555_pin_write(uint16_t pin, int level)
{
    uint8_t state[2];

    ESP_RETURN_ON_ERROR(xl9555_read_ports(state), TAG, "xl9555 read failed");

    if (pin <= 0x00FF) {
        if (level) {
            state[0] |= (uint8_t)pin;
        } else {
            state[0] &= (uint8_t)~pin;
        }
    } else {
        uint8_t mask = (uint8_t)(pin >> 8);
        if (level) {
            state[1] |= mask;
        } else {
            state[1] &= (uint8_t)~mask;
        }
    }

    return xl9555_write_reg(XL9555_OUTPUT_PORT0_REG, state, sizeof(state));
}

static esp_err_t board_i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA_IO,
        .scl_io_num = BOARD_I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
        },
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_i2c_bus), TAG, "i2c bus init failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = XL9555_I2C_ADDR,
        .scl_speed_hz = BOARD_I2C_FREQ_HZ,
    };

    return i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_xl9555_dev);
}

static esp_err_t xl9555_init(void)
{
    uint8_t cfg[2] = {
        (uint8_t)(XL9555_CONFIG_DEFAULT & 0xFF),
        (uint8_t)(XL9555_CONFIG_DEFAULT >> 8),
    };
    uint8_t clear_irq_state[2];

    ESP_RETURN_ON_ERROR(xl9555_read_ports(clear_irq_state), TAG, "xl9555 probe failed");
    ESP_RETURN_ON_ERROR(xl9555_write_reg(XL9555_CONFIG_PORT0_REG, cfg, sizeof(cfg)), TAG, "xl9555 config failed");

    /* Match the board BSP default state: speaker amp disabled, beep idle high. */
    ESP_RETURN_ON_ERROR(xl9555_pin_write(XL9555_SPK_EN_IO, 1), TAG, "speaker disable failed");
    ESP_RETURN_ON_ERROR(xl9555_pin_write(XL9555_BEEP_IO, 1), TAG, "beep idle failed");

    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(board_i2c_init());
    ESP_ERROR_CHECK(xl9555_init());

    ESP_LOGI(TAG, "board beep test started");
    ESP_LOGI(TAG, "pattern: %d ms on, %d ms off", BEEP_ON_TIME_MS, BEEP_OFF_TIME_MS);

    while (1) {
        ESP_ERROR_CHECK(xl9555_pin_write(XL9555_BEEP_IO, 0));
        vTaskDelay(pdMS_TO_TICKS(BEEP_ON_TIME_MS));

        ESP_ERROR_CHECK(xl9555_pin_write(XL9555_BEEP_IO, 1));
        vTaskDelay(pdMS_TO_TICKS(BEEP_OFF_TIME_MS));
    }
}
