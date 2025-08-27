/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_bridge_event {
  int dummy_var;
};

ZMK_EVENT_DECLARE(zmk_bridge_event);