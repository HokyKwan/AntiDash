/*
 * Copyright (c) 2026 HokyKwan <hokykwan0@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "led_svc.h"

#include <errno.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_svc);

#define BT_UUID_LED_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_LED_RGB_VAL \
    BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)

static const struct bt_uuid_128 led_svc_uuid = BT_UUID_INIT_128(BT_UUID_LED_SERVICE_VAL);
static const struct bt_uuid_128 led_rgb_uuid = BT_UUID_INIT_128(BT_UUID_LED_RGB_VAL);

static uint8_t rgb[3];
static struct ad_led_svc_cb led_cb;

static int on_notify(uint8_t r, uint8_t g, uint8_t b);

static ssize_t on_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, rgb, sizeof(rgb));
}

static ssize_t on_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
             const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    const uint8_t *data = buf;

    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (len != sizeof(rgb)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    (void)on_notify(data[0], data[1], data[2]);
    return len;
}

static void on_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    ARG_UNUSED(attr);

    LOG_INF("[LED SVC] notify %s", (value == BT_GATT_CCC_NOTIFY) ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(led_svc,
    BT_GATT_PRIMARY_SERVICE(&led_svc_uuid),
    BT_GATT_CHARACTERISTIC(&led_rgb_uuid.uuid,
                   BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
                   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                   on_read, on_write, NULL),
    BT_GATT_CCC(on_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static int on_notify(uint8_t r, uint8_t g, uint8_t b)
{
    const struct bt_gatt_attr *attr;
    int err;

    rgb[0] = r;
    rgb[1] = g;
    rgb[2] = b;

    if (led_cb.on_rgb != NULL) {
        led_cb.on_rgb(r, g, b);
    }

    attr = bt_gatt_find_by_uuid(led_svc.attrs, (uint16_t)led_svc.attr_count,
                    &led_rgb_uuid.uuid);
    if (attr == NULL) {
        return -ENOENT;
    }

    err = bt_gatt_notify(NULL, attr, rgb, sizeof(rgb));
    if (err == -ENOTCONN) {
        return 0;
    }

    if (err != 0) {
        LOG_WRN("[LED SVC] notify failed (err %d)", err);
    }

    return err;
}

int ad_led_svc_init(const struct ad_led_svc_cb *cb)
{
    if (cb == NULL) {
        return -EINVAL;
    }

    led_cb = *cb;
    return 0;
}

int ad_led_svc_notify(uint8_t r, uint8_t g, uint8_t b)
{
    return on_notify(r, g, b);
}
