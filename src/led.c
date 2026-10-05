/*
 * Copyright (c) 2026 HokyKwan <hokykwan0@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "led.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(led);

#define STRIP_NODE DT_ALIAS(rgb_led)

#if DT_NODE_HAS_PROP(DT_ALIAS(rgb_led), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(rgb_led), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static struct led_rgb pixels[STRIP_NUM_PIXELS];
static struct led_rgb current = RGB(0x00, 0x00, 0x00);
static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static K_MUTEX_DEFINE(lock);

static void set_rgb(void)
{
    int err;

    for (size_t i = 0; i < ARRAY_SIZE(pixels); i++) {
        pixels[i] = current;
    }

    err = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
    if (err != 0) {
        LOG_ERR("[LED] couldn't update strip: %d", err);
    }
}

void ad_led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (!device_is_ready(strip)) {
        LOG_ERR("[LED] strip device %s is not ready", strip->name);
        return;
    }

    k_mutex_lock(&lock, K_FOREVER);
    current.r = r;
    current.g = g;
    current.b = b;
    set_rgb();
    k_mutex_unlock(&lock);
}
