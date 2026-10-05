/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ble.h"
#include "led.h"
#include "led_svc.h"

LOG_MODULE_REGISTER(main);

static const struct ad_led_svc_cb led_cb = {
    .on_rgb = ad_led_set_rgb,
};

int main(void)
{
    int err = ad_led_svc_init(&led_cb);

    if (err) {
        LOG_ERR("LED service init failed (err %d)", err);
    }

    err = ad_ble_start();
    if (err) {
        LOG_ERR("Bluetooth start failed (err %d)", err);
    }

    return 0;
}
