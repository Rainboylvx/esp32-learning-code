#pragma once
#include "host/ble_hs.h"
#include "host/ble_uuid.h"

// 蓝牙数据接收回调函数类型定义
typedef void (*ble_data_callback_t)(uint8_t *data, uint16_t len);

/**
 * @brief 注册蓝牙数据接收回调
 * @param cb 回调函数指针，接收数据时被调用
 */
void ble_register_data_callback(ble_data_callback_t cb);

int gatt_svr_init(void);
int gatt_svr_chr_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt *ctxt, void *arg);
int ble_gap_event_cb(struct ble_gap_event *event, void *arg);
void ble_app_advertise(void);
void ble_app_on_sync(void);
int gatt_svr_chr_access_notify_cccd(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg);