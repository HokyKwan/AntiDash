/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ble.h"
#include "led.h"
#include "led_svc.h"
#include "rader.h"

LOG_MODULE_REGISTER(main);

static const struct ad_led_svc_cb led_cb = {
    .on_rgb = ad_led_set_rgb,
};

static void on_rader_rx(const uint8_t *data, size_t len)
{
    uint8_t rgb[3];

    LOG_INF("[MAIN] received %u bytes: %.*s", (unsigned int)len, (int)len, data);

    if (len > 0U) {
        ad_rader_send(data, len);
        ad_rader_send((const uint8_t *)"\r\n", 2);
    }

    if ((len == 6U) &&
        (sscanf((const char *)data, "%2hhx%2hhx%2hhx", &rgb[0], &rgb[1], &rgb[2]) == 3)) {
        ad_led_svc_notify(rgb[0], rgb[1], rgb[2]);
    }
}

int main(void)
{
    int err = ad_led_svc_init(&led_cb);
    if (err) {
        LOG_ERR("[MAIN] led service init failed (err %d)", err);
    }

    err = ad_ble_init();
    if (err) {
        LOG_ERR("[MAIN] bluetooth init failed (err %d)", err);
    }

    err = ad_rader_init(on_rader_rx);
    if (err) {
        LOG_ERR("[MAIN] radar init failed (err %d)", err);
    }

    return 0;
}
