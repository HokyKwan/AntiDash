/*
 * Copyright (c) 2026 HokyKwan <hokykwan0@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LED_SVC_H
#define LED_SVC_H

#include <stdint.h>

struct ad_led_svc_cb {
    void (*on_rgb)(uint8_t r, uint8_t g, uint8_t b);
};

int ad_led_svc_init(const struct ad_led_svc_cb *cb);

#endif
