/*
 * Copyright (c) 2026 HokyKwan <hokykwan0@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RADER_H
#define RADER_H

#include <stddef.h>
#include <stdint.h>

typedef void (*rader_rx_cb_t)(const uint8_t *data, size_t len);

int ad_rader_init(rader_rx_cb_t on_rx);

int ad_rader_send(const uint8_t *data, size_t len);

#endif
