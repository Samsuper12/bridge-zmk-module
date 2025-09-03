/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

// https://github.com/zmkfirmware/zmk-studio-messages/blob/main/proto/zmk/core.proto

#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/debug/thread_analyzer.h>

#include <pb_decode.h>
#include <pb_encode.h>
#include <proto/core.pb.h>

#include <uart_framing.h>

LOG_MODULE_REGISTER(zmk_bridge, CONFIG_ZMK_BRIDGE_LOG_LEVEL);

#include <zmk/bridge.h>
#include <zmk/events/bridge_event.h>

#define UART_DEVICE_NODE DT_CHOSEN(zmk_bridge_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

RING_BUF_DECLARE(bridge_rx_buf, CONFIG_ZMK_BRIDGE_RX_BUF_SIZE);
RING_BUF_DECLARE(bridge_tx_buf, CONFIG_ZMK_BRIDGE_TX_BUF_SIZE);

static K_SEM_DEFINE(bridge_rx_sem, 0, 1);
static K_MUTEX_DEFINE(bridge_transport_mutex);

static enum uart_framing_state bridge_framing_state;

static void tx_notify(struct ring_buf *tx_ring_buf, size_t written, bool msg_done,
                      void *user_data) {
    if (msg_done || (ring_buf_size_get(tx_ring_buf) > (ring_buf_capacity_get(tx_ring_buf) / 2))) {
#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
        uart_irq_tx_enable(uart_dev);
#else
        struct ring_buf *tx_buf = &bridge_tx_buf;
        uint8_t *buf;
        uint32_t claim_len;
        while ((claim_len = ring_buf_get_claim(tx_buf, &buf, tx_buf->size)) > 0) {
            for (int i = 0; i < claim_len; i++) {
                uart_poll_out(uart_dev, buf[i]);
            }

            ring_buf_get_finish(tx_buf, claim_len);
        }
#endif
    }
}

static bool bridge_read_cb(pb_istream_t *stream, uint8_t *buf, size_t count) {
    uint32_t write_offset = 0;

    do {
        uint8_t *buffer;
        uint32_t len = ring_buf_get_claim(&bridge_rx_buf, &buffer, count);

        if (len > 0) {
            for (int i = 0; i < len; i++) {
                if (uart_framing_process_byte(&bridge_framing_state,
                                              buffer[i])) { // get it from studio
                    buf[write_offset++] = buffer[i];
                }
            }
        } else {
            k_sem_take(&bridge_rx_sem, K_FOREVER);
        }

        ring_buf_get_finish(&bridge_rx_buf, len);
    } while (write_offset < count && bridge_framing_state != FRAMING_STATE_EOF);

    if (bridge_framing_state == FRAMING_STATE_EOF) {
        stream->bytes_left = 0;
        return false;
    } else {
        return true;
    }
}

static bool bridge_tx_buffer_write(pb_ostream_t *stream, const uint8_t *buf, size_t count) {
    void *user_data = stream->state;
    size_t written = 0;

    bool escape_byte_already_written = false;
    do {
        uint32_t write_idx = 0;

        uint8_t *write_buf;
        uint32_t claim_len = ring_buf_put_claim(&bridge_tx_buf, &write_buf, count - written);

        if (claim_len == 0) {
            continue;
        }

        int escapes_written = 0;
        for (int i = 0; i < claim_len && write_idx < claim_len; i++) {
            uint8_t b = buf[written + i];
            switch (b) {
            case FRAMING_EOF:
            case FRAMING_ESC:
            case FRAMING_SOF:
                // Care to be taken. We may need to write the escape byte,
                // but that's the last available spot for this claim, so we track
                // if the escape has already been written in the previous iteration
                // of our loop.
                if (!escape_byte_already_written) {
                    escapes_written++;
                    write_buf[write_idx++] = FRAMING_ESC;
                    escape_byte_already_written = true;
                    if (write_idx >= claim_len) {
                        LOG_WRN("Skipping on, no room to write escape and real byte");
                        continue;
                    }
                }
            default:
                write_buf[write_idx++] = b;
                escape_byte_already_written = false;
                break;
            }
        }

        ring_buf_put_finish(&bridge_tx_buf, write_idx);

        written += (write_idx - escapes_written);

        tx_notify(&bridge_tx_buf, write_idx, false, user_data);
    } while (written < count);

    return true;
}

static pb_istream_t pb_istream_for_rx_ring_buf() {
    pb_istream_t stream = {&bridge_read_cb, NULL, SIZE_MAX};
    return stream;
}

static pb_ostream_t pb_ostream_for_tx_buf(void *user_data) {
    pb_ostream_t stream = {&bridge_tx_buffer_write, (void *)user_data, SIZE_MAX, 0};
    return stream;
}

static int send_response(const bridge_core_Response *resp) {
    k_mutex_lock(&bridge_transport_mutex, K_FOREVER);
    LOG_INF("Sensing response.");

    // if (!selected_transport) {
    //   goto exit;
    //}

    // void *user_data = selected_transport->tx_user_data ? selected_transport->tx_user_data() :
    // NULL;

    void *user_data = NULL;
    pb_ostream_t stream = pb_ostream_for_tx_buf(user_data);

    uint8_t framing_byte = FRAMING_SOF;
    ring_buf_put(&bridge_tx_buf, &framing_byte, 1);

    tx_notify(&bridge_tx_buf, 1, false, user_data);

    /* Now we are ready to encode the message! */
    bool status = pb_encode(&stream, &bridge_core_Response_msg, resp);

    if (!status) {
#if !IS_ENABLED(CONFIG_NANOPB_NO_ERRMSG)
        LOG_ERR("Failed to encode the message %s", stream.errmsg);
#endif // !IS_ENABLED(CONFIG_NANOPB_NO_ERRMSG)
        return -EINVAL;
    }

    framing_byte = FRAMING_EOF;
    ring_buf_put(&bridge_tx_buf, &framing_byte, 1);

    tx_notify(&bridge_tx_buf, 1, true, user_data);

exit:
    k_mutex_unlock(&bridge_transport_mutex);
    return 0;
}

// FIXME:
static bridge_core_Response handle_request(const bridge_core_Request *req) {
    LOG_INF("New request.");

    // zmk_studio_core_reschedule_lock_timeout();
    // struct zmk_rpc_subsystem *sub = find_subsystem_for_choice(req->which_subsystem);
    // if (!sub) {
    // LOG_WRN("No subsystem found for choice %d", req->which_subsystem);
    //  return ZMK_RPC_RESPONSE(meta, simple_error, zmk_meta_ErrorConditions_RPC_NOT_FOUND);
    // }

    // zmk_studio_Response resp = sub->func(sub, req);
    // resp.type.request_response.request_id = req->request_id;

    bridge_core_Response resp = bridge_core_Response_init_zero;

    return resp;
}

static void bridge_main(void) {
    LOG_INF("Thread start.");

    for (;;) {
        pb_istream_t stream = pb_istream_for_rx_ring_buf();
        bridge_core_Request req = bridge_core_Request_init_zero;
#if IS_ENABLED(CONFIG_THREAD_ANALYZER)
        thread_analyzer_print();
#endif // IS_ENABLED(CONFIG_THREAD_ANALYZER)
        bool status = pb_decode(&stream, &bridge_core_Request_msg, &req);

        bridge_framing_state = FRAMING_STATE_IDLE;

        if (status) {
            bridge_core_Response resp = handle_request(&req);

            int err = send_response(&resp);
#if IS_ENABLED(CONFIG_THREAD_ANALYZER)
            thread_analyzer_print();
#endif // IS_ENABLED(CONFIG_THREAD_ANALYZER)
            if (err < 0) {
                LOG_ERR("Failed to send the Bridge response %d", err);
            }
        } else {
            LOG_DBG("Decode failed");
        }
    }
}

K_THREAD_DEFINE(bridge_thread, CONFIG_ZMK_BRIDGE_THREAD_STACK_SIZE, bridge_main, NULL, NULL, NULL,
                K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
static void serial_cb(const struct device *dev, void *user_data) {
    if (!uart_irq_update(uart_dev)) {
        return;
    }

    if (uart_irq_rx_ready(uart_dev)) {
        /* read until FIFO empty */
        uint32_t last_read = 0, len = 0;
        struct ring_buf *buf = &bridge_rx_buf;
        do {
            uint8_t *buffer;
            len = ring_buf_put_claim(buf, &buffer, buf->size);
            if (len > 0) {
                last_read = uart_fifo_read(uart_dev, buffer, len);

                ring_buf_put_finish(buf, last_read);
            } else {
                LOG_ERR("Dropping incoming bridge byte, insufficient room in the RX "
                        "buffer. Bump "
                        "ZMK_BRIDGE_TRANSPORT_UART_RX_STACK_SIZE.");
                uint8_t dummy;
                last_read = uart_fifo_read(uart_dev, &dummy, 1);
            }
        } while (last_read && last_read == len);

        k_sem_give(&bridge_rx_sem);
    }

    if (uart_irq_tx_ready(uart_dev)) {
        struct ring_buf *tx_buf = &bridge_tx_buf;
        uint32_t len;
        while ((len = ring_buf_size_get(tx_buf)) > 0) {
            uint8_t *buf;
            uint32_t claim_len = ring_buf_get_claim(tx_buf, &buf, tx_buf->size);

            if (claim_len == 0) {
                continue;
            }

            int sent = uart_fifo_fill(uart_dev, buf, claim_len);

            ring_buf_get_finish(tx_buf, MAX(sent, 0));
        }
    }
}

#else // IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)

static void uart_rx_main(void) {
    for (;;) {
        uint8_t *buf;
        struct ring_buf *ring_buf = &bridge_rx_buf;
        uint32_t claim_len = ring_buf_put_claim(ring_buf, &buf, 1);

        if (claim_len < 1) {
            LOG_WRN("NO CLAIM ABLE TO BE HAD");
            k_sleep(K_MSEC(1));
            continue;
        }

        if (uart_poll_in(uart_dev, buf) < 0) {
            ring_buf_put_finish(ring_buf, 0);
            k_sleep(K_MSEC(1));
        } else {
            ring_buf_put_finish(ring_buf, 1);
            k_sem_give(&bridge_rx_sem);
        }
    }
}

K_THREAD_DEFINE(uart_transport_read_thread, CONFIG_ZMK_BRIDGE_TRANSPORT_UART_RX_STACK_SIZE,
                uart_rx_main, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

#endif //! IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)

static int start_rx() {
#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
    uart_irq_rx_enable(uart_dev);
#else
    k_thread_resume(uart_transport_read_thread);
#endif
    return 0;
}

static int stop_rx(void) {
#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
    uart_irq_rx_disable(uart_dev);
#else
    k_thread_suspend(uart_transport_read_thread);
#endif
    return 0;
}

// ZMK_RPC_TRANSPORT(uart, ZMK_TRANSPORT_USB, start_rx, stop_rx, NULL, tx_notify);

static int uart_bridge_interface_init(void) {
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not found!");
        return -ENODEV;
    }

    LOG_INF("UART start.");

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

    if (ret < 0) {
        if (ret == -ENOTSUP) {
            printk("Interrupt-driven UART API support not enabled\n");
        } else if (ret == -ENOSYS) {
            printk("UART device does not support interrupt-driven API\n");
        } else {
            printk("Error setting UART callback: %d\n", ret);
        }
        return ret;
    }
#endif // IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)

    return 0;
}

SYS_INIT(uart_bridge_interface_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
