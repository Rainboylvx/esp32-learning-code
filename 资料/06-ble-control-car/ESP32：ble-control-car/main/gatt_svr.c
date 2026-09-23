#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "host/util/util.h"
#include "esp_mac.h"

#include "gatt_svr.h"
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
#include "esp_log.h"
#endif

#define TAG "BLE_SVR"

/* UUIDs provided by user */
// 服务UUID: 00008000-0000-1000-8000-57616C6B697A
const ble_uuid128_t gatt_svr_svc_car_uuid = BLE_UUID128_INIT(
    0x7A, 0x69, 0x6B, 0x6C, 0x61, 0x57, 0x00, 0x80,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00);

// 写特征UUID: 00008001-0000-1000-8000-57616C6B697A
const ble_uuid128_t gatt_svr_chr_write_uuid = BLE_UUID128_INIT(
    0x7A, 0x69, 0x6B, 0x6C, 0x61, 0x57, 0x00, 0x80,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00);

// 通知特征UUID: 00008002-0000-1000-8000-57616C6B697A
const ble_uuid128_t gatt_svr_chr_notify_uuid = BLE_UUID128_INIT(
    0x7A, 0x69, 0x6B, 0x6C, 0x61, 0x57, 0x00, 0x80,
    0x10, 0x00, 0x00, 0x00, 0x02, 0x80, 0x00, 0x00);

uint16_t gatt_svr_chr_notify_val_handle;
uint16_t gatt_svr_chr_write_val_handle;
uint16_t g_notify_conn_handle = BLE_HS_CONN_HANDLE_NONE;
// CCCD UUID
const ble_uuid16_t gatt_dsc_cccd_uuid = BLE_UUID16_INIT(0x2902);

// 静态全局回调函数指针
static ble_data_callback_t s_ble_data_callback = NULL;

/**
 * @brief 注册蓝牙数据接收回调
 */
void ble_register_data_callback(ble_data_callback_t cb)
{
    s_ble_data_callback = cb;
}

const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_car_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){{
                                                           .uuid = &gatt_svr_chr_write_uuid.u,
                                                           .access_cb = gatt_svr_chr_write_cb,
                                                           .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_INDICATE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                                                           .val_handle = &gatt_svr_chr_write_val_handle,
                                                       },
                                                       {
                                                           .uuid = &gatt_svr_chr_notify_uuid.u,
                                                           .access_cb = gatt_svr_chr_access,
                                                           .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_INDICATE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                                                           .val_handle = &gatt_svr_chr_notify_val_handle,
                                                           .descriptors = (struct ble_gatt_dsc_def[]){
                                                               {
                                                                   // CCCD 描述符
                                                                   .uuid = &gatt_dsc_cccd_uuid.u,
                                                                   .att_flags = BLE_ATT_F_READ,
                                                                   .access_cb = gatt_svr_chr_access_notify_cccd,
                                                               },
                                                               {0}, /* 描述符列表结束 */
                                                           },
                                                       },
                                                       {
                                                           0, /* No more characteristics in this service */
                                                       }},
    },
    {
        0, /* No more services */
    },
};
/**
 * CCCD 访问回调（处理通知的启用/禁用）
 */
int gatt_svr_chr_access_notify_cccd(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // 读取CCCD值
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
    {
        uint16_t cccd_val = (g_notify_conn_handle != BLE_HS_CONN_HANDLE_NONE) ? 0x0001 : 0x0000;
        uint8_t data[2] = {cccd_val & 0xFF, (cccd_val >> 8) & 0xFF};

        ctxt->om = ble_hs_mbuf_from_flat(data, sizeof(data));
        if (ctxt->om == NULL)
        {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return 0;
    }

    // 写入CCCD值（启用/禁用通知）
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        uint16_t cccd_val = 0;
        uint16_t data_len = OS_MBUF_PKTLEN(ctxt->om);

        if (data_len >= 2)
        {
            uint8_t data[2];
            int rc = ble_hs_mbuf_to_flat(ctxt->om, data, sizeof(data), NULL);
            if (rc == 0)
            {
                cccd_val = data[0] | (data[1] << 8);

                // 保存启用了通知的连接句柄
                if (cccd_val & 0x0001)
                {
                    g_notify_conn_handle = conn_handle;
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
                    ESP_LOGI(TAG, "通知已启用, conn_handle=%d", conn_handle);
#endif
                }
                else
                {
                    g_notify_conn_handle = BLE_HS_CONN_HANDLE_NONE;
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
                    ESP_LOGI(TAG, "通知已禁用");
#endif
                }
            }
        }
        return 0;
    }

    return 0;
}

// 蓝牙事件处理（你原来的逻辑）
int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
        ESP_LOGI(TAG, "蓝牙已连接 (conn_handle: %d)", event->connect.conn_handle);
#endif
        break;

    case BLE_GAP_EVENT_DISCONNECT:
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
        ESP_LOGI(TAG, "蓝牙已断开连接，原因: %d", event->disconnect.reason);
#endif
        ble_app_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:

#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
        ESP_LOGI(TAG, "广播完成，重新开始...");
#endif
        ble_app_advertise();
        break;

    default:
        break;
    }
    return 0;
}

void ble_app_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;

    int rc;

    memset(&fields, 0, sizeof fields);
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)"ESP32_CAR";
    fields.name_len = strlen("ESP32_CAR");
    fields.name_is_complete = 1;

    uint8_t mac[6];

    esp_err_t ret = esp_read_mac(mac, ESP_MAC_BT);
    if (ret != ESP_OK)
    {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
        ESP_LOGE(TAG, "读取芯片MAC失败: %s", esp_err_to_name(ret));
#endif
        return;
    }

    uint8_t mfg_data[8] = {
        0xFF, 0xFF, // 厂商ID (0xFFFF)，小端序
        mac[0], mac[1], mac[2],
        mac[3], mac[4], mac[5]};

    fields.mfg_data = mfg_data;
    fields.mfg_data_len = sizeof(mfg_data);

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
        ESP_LOGE(TAG, "error setting advertisement data; rc=%d", rc);
#endif
        return;
    }

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event_cb, NULL);
    if (rc != 0)
    {
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
        ESP_LOGE(TAG, "error enabling advertisement; rc=%d", rc);
#endif
        return;
    }
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
    ESP_LOGI(TAG, "Advertisement started");
#endif
}

void ble_app_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Begin advertising. */
    ble_app_advertise();
}

int gatt_svr_chr_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid;
    int rc;

    uuid = ctxt->chr->uuid;

    if (ble_uuid_cmp(uuid, &gatt_svr_chr_write_uuid.u) == 0)
    {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
        {
            /* Handle write */
            uint8_t buf[512];
            uint16_t len = ctxt->om->om_len;
            if (len > sizeof(buf) - 1)
            {
                len = sizeof(buf) - 1;
            }

            rc = os_mbuf_copydata(ctxt->om, 0, len, buf);
            if (rc == 0)
            {
                if (s_ble_data_callback != NULL)
                {
                    s_ble_data_callback(buf, len);
                }
            }
            return 0;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid;
    int rc;

    uuid = ctxt->chr->uuid;

    if (ble_uuid_cmp(uuid, &gatt_svr_chr_write_uuid.u) == 0)
    {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
        {
            /* Handle write */
            char buf[256];
            uint16_t len = ctxt->om->om_len;
            if (len > sizeof(buf) - 1)
            {
                len = sizeof(buf) - 1;
            }

            rc = os_mbuf_copydata(ctxt->om, 0, len, buf);
            if (rc == 0)
            {
                buf[len] = '\0';
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
                ESP_LOGI(TAG, "gatt_svr_chr_access: %s\n", buf);
#endif
            }
            return 0;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

int gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
    ESP_LOGI(TAG, "ble_gatts_count_cfg %d", rc);
#endif
    if (rc != 0)
    {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
    ESP_LOGI(TAG, "ble_gatts_add_svcs %d", rc);
#endif
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}
