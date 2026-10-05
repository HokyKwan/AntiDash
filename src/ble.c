/*
 * Copyright (c) 2026 HokyKwan <hokykwan0@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble.h"

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BT_UUID_LED_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_LED_RGB_VAL \
    BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)

static const struct bt_uuid_128 led_svc_uuid = BT_UUID_INIT_128(BT_UUID_LED_SERVICE_VAL);
static const struct bt_uuid_128 led_rgb_uuid = BT_UUID_INIT_128(BT_UUID_LED_RGB_VAL);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static uint8_t rgb[3];
static struct ad_ble_cb ble_cb;

static ssize_t on_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, rgb, sizeof(rgb));
}

static ssize_t on_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
             const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    const uint8_t *data = buf;
    int err;

    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (len != sizeof(rgb)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    memcpy(rgb, data, sizeof(rgb));

    if (ble_cb.on_rgb != NULL) {
        ble_cb.on_rgb(rgb[0], rgb[1], rgb[2]);
    }

    err = bt_gatt_notify(NULL, attr, rgb, sizeof(rgb));
    if ((err != 0) && (err != -ENOTCONN)) {
        LOG_WRN("[BLE] notify failed (err %d)", err);
    }

    return len;
}

static void on_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    ARG_UNUSED(attr);

    LOG_INF("[BLE] notify %s", (value == BT_GATT_CCC_NOTIFY) ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(led_svc,
    BT_GATT_PRIMARY_SERVICE(&led_svc_uuid),
    BT_GATT_CHARACTERISTIC(&led_rgb_uuid.uuid,
                   BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
                   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                   on_read, on_write, NULL),
    BT_GATT_CCC(on_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

int ad_ble_start(const struct ad_ble_cb *cb)
{
    int err;

    if (cb == NULL) {
        return -EINVAL;
    }

    ble_cb = *cb;

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("[BLE] init failed (err %d)", err);
        return err;
    }

    LOG_INF("[BLE] initialized");

    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("[BLE] advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("[BLE] advertising started as %s", DEVICE_NAME);
    return 0;
}
