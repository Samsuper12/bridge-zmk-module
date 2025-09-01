/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
// #include <zephyr/logging/log.h>

enum uart_raming_state {
    FRAMING_STATE_IDLE,
    FRAMING_STATE_AWAITING_DATA,
    FRAMING_STATE_ESCAPED,
    FRAMING_STATE_ERR,
    FRAMING_STATE_EOF,
};

#define FRAMING_SOF 0xAB
#define FRAMING_ESC 0xAC
#define FRAMING_EOF 0xAD

static inline bool _process_byte_err_state(enum uart_raming_state *uart_fs, uint8_t c) {
    switch (c) {
    case FRAMING_EOF:
        *uart_fs = FRAMING_STATE_IDLE;
        return false;
    case FRAMING_SOF:
        *uart_fs = FRAMING_STATE_AWAITING_DATA;
        return false;
    default:
        // LOG_WRN("Discarding unexpected data 0x%02x", c);
        return false;
    }

    return false;
}

static inline bool _process_byte_idle_state(enum uart_raming_state *uart_fs, uint8_t c) {
    switch (c) {
    case FRAMING_SOF:
        *uart_fs = FRAMING_STATE_AWAITING_DATA;
        return false;
    default:
        // LOG_WRN("Expected SOF, got 0x%02x", c);
        return false;
    }
    return false;
}

static inline bool _process_byte_awaiting_data_state(enum uart_raming_state *uart_fs, uint8_t c) {
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

static inline bool _process_byte_escaped_state(enum uart_raming_state *uart_fs, uint8_t c) {
    *uart_fs = FRAMING_STATE_AWAITING_DATA;
    return true;
}

/**
 * @brief Process an incoming byte from a frame. Will possibly update the framing state depending on
 * what data is received.
 * @retval true if data is a non-framing byte, and is real data to be handled by the upper level
 * logic.
 * @retval false if data is a framing byte, and should be ignored. Also indicates the framing state
 * has been updated.
 */
static inline bool uart_framing_process_byte(enum uart_raming_state *uart_fs, uint8_t c) {
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
        // LOG_ERR("Unsupported framing state: %d", *uart_fs);
        return false;
    }
}
