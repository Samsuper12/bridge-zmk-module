/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <uart_framing.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static bool _process_byte_err_state(enum uart_framing_state *uart_fs, uint8_t c) {
    switch (c) {
    case FRAMING_EOF:
        *uart_fs = FRAMING_STATE_IDLE;
        return false;
    case FRAMING_SOF:
        *uart_fs = FRAMING_STATE_AWAITING_DATA;
        return false;
    default:
        LOG_WRN("Discarding unexpected data 0x%02x", c);
        return false;
    }

    return false;
}

static bool _process_byte_idle_state(enum uart_framing_state *uart_fs, uint8_t c) {
    switch (c) {
    case FRAMING_SOF:
        *uart_fs = FRAMING_STATE_AWAITING_DATA;
        return false;
    default:
        LOG_WRN("Expected SOF, got 0x%02x", c);
        return false;
    }
    return false;
}

static bool _process_byte_awaiting_data_state(enum uart_framing_state *uart_fs, uint8_t c) {
    switch (c) {
    case FRAMING_SOF:
        // LOG_WRN("Unescaped SOF mid-data");
        *uart_fs = FRAMING_STATE_ERR;
        return false;
    case FRAMING_ESC:
        *uart_fs = FRAMING_STATE_ESCAPED;
        return false;
    case FRAMING_EOF:
        *uart_fs = FRAMING_STATE_EOF;
        return false;
    default:
        return true;
    }

    return false;
}

static inline bool _process_byte_escaped_state(enum uart_framing_state *uart_fs, uint8_t c) {
    *uart_fs = FRAMING_STATE_AWAITING_DATA;
    return true;
}

bool uart_framing_process_byte(enum uart_framing_state *uart_fs, uint8_t c) {
    switch (*uart_fs) {
    case FRAMING_STATE_ERR:
        return _process_byte_err_state(uart_fs, c);
    case FRAMING_STATE_IDLE:
    case FRAMING_STATE_EOF:
        return _process_byte_idle_state(uart_fs, c);
    case FRAMING_STATE_AWAITING_DATA:
        return _process_byte_awaiting_data_state(uart_fs, c);
    case FRAMING_STATE_ESCAPED:
        return _process_byte_escaped_state(uart_fs, c);
    default:
        LOG_ERR("Unsupported framing state: %d", *uart_fs);
        return false;
    }
}