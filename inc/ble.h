/*
 * Copyright (c) 2026 HokyKwan <hokykwan0@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_H
#define BLE_H

#include <stdint.h>

struct ad_ble_cb {
    void (*on_rgb)(uint8_t r, uint8_t g, uint8_t b);
};

int ad_ble_start(const struct ad_ble_cb *cb);

#endif
