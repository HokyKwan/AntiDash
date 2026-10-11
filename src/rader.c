/*
 * Copyright (c) 2026 HokyKwan <hokykwan0@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rader.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rader_svc);

#define RADER_NODE DT_ALIAS(rader_uart)
#define UART_LINE_MAX 128
#define UART_QUEUE_DEPTH 8

K_MSGQ_DEFINE(rx_q, UART_LINE_MAX, UART_QUEUE_DEPTH, 4);

static const struct device *const rader_dev = DEVICE_DT_GET(RADER_NODE);
static rader_rx_cb_t rx_cb;
static uint8_t rx_buf[UART_LINE_MAX];
static size_t rx_len;
static bool started;

K_MUTEX_DEFINE(tx_lock);

static void deliver(void)
{
    if (rx_len == 0U) {
        return;
    }

    rx_buf[rx_len] = '\0';

    if (k_msgq_put(&rx_q, rx_buf, K_NO_WAIT) != 0) {
        LOG_WRN("RX queue full, dropped %u bytes", (unsigned int)rx_len);
    }

    rx_len = 0U;
}

static void uart_isr(const struct device *dev, void *user_data)
{
    uint8_t byte;

    ARG_UNUSED(user_data);

    uart_irq_update(dev);

    if (uart_irq_rx_ready(dev) <= 0) {
        return;
    }

    while (uart_fifo_read(dev, &byte, 1) == 1) {
        if ((byte == '\n') || (byte == '\r')) {
            deliver();
            continue;
        }

        if (rx_len >= (UART_LINE_MAX - 1U)) {
            deliver();
        }

        rx_buf[rx_len++] = byte;
    }
}

static void uart_thread(void *a, void *b, void *c)
{
    uint8_t line[UART_LINE_MAX];

    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    while (k_msgq_get(&rx_q, line, K_FOREVER) == 0) {
        size_t len = strnlen((char *)line, UART_LINE_MAX - 1U);

        LOG_INF("[RADER] received %u bytes: %.*s", (unsigned int)len, (int)len, line);

        if (rx_cb != NULL) {
            rx_cb(line, len);
        }
    }
}

K_THREAD_DEFINE(uart_tid, 2048, uart_thread, NULL, NULL, NULL, 7, 0, 0);

int ad_rader_init(rader_rx_cb_t on_rx)
{
    int err;

    if (!device_is_ready(rader_dev)) {
        LOG_ERR("[RADER] device %s is not ready", rader_dev->name);
        return -ENODEV;
    }

    rx_cb = on_rx;

    err = uart_irq_callback_user_data_set(rader_dev, uart_isr, NULL);
    if (err != 0) {
        LOG_ERR("[RADER] callback setup failed (err %d)", err);
        return err;
    }

    uart_irq_rx_enable(rader_dev);
    started = true;
    LOG_INF("[RADER] ready, RX GPIO4 TX GPIO14, 115200 8N1");
    return 0;
}

int ad_rader_send(const uint8_t *data, size_t len)
{
    if (data == NULL) {
        return -EINVAL;
    }

    if (len == 0U) {
        return 0;
    }

    if (!started || !device_is_ready(rader_dev)) {
        return -ENODEV;
    }

    if (k_is_in_isr()) {
        return -EPERM;
    }

    k_mutex_lock(&tx_lock, K_FOREVER);

    for (size_t i = 0U; i < len; i++) {
        uart_poll_out(rader_dev, data[i]);
    }

    k_mutex_unlock(&tx_lock);
    return 0;
}
