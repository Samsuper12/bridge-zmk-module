/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zmk/bridge.h>
#include <zmk/events/bridge_event.h>

LOG_MODULE_DECLARE(zmk_bridge, CONFIG_ZMK_STUDIO_LOG_LEVEL);

#define UART_DEVICE_NODE DT_CHOSEN(zmk_bridge_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)

static int uart_bridge_interface_init(void) {
  if (!device_is_ready(uart_dev)) {
    LOG_ERR("UART device not found!");
    return -ENODEV;
  }

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

SYS_INIT(uart_bridge_interface_init, POST_KERNEL,
         CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
