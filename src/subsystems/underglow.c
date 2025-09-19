/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/bridge.h>
#include <pb_encode.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zmk/behavior.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/virtual_key_position.h>

LOG_MODULE_DECLARE(zmk_bridge, CONFIG_ZMK_LOG_LEVEL);

static const struct device *rgb_ug_dev = DEVICE_DT_GET(DT_NODELABEL(rgb_ug));

static struct zmk_behavior_binding binding = {
    .behavior_dev = NULL,
    .param1 = 0,
    .param2 = 0,
};

bridge_Response set_ug_cmd(const bridge_Request *req) {
    if (!device_is_ready(rgb_ug_dev)) {
        LOG_ERR("The rgb_ug node cannot be found!");
        BRIDGE_RESPONSE_SIMPLE(false);
    }

    struct zmk_behavior_binding_event event = {
        .position = 0,
        .timestamp = k_uptime_get(),
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };

    binding.behavior_dev = rgb_ug_dev;
    binding.param1 = req->subsystem.underglow.request_type.set_ug_cmd;
    LOG_WRN("param1 value: %d", binding.param1);

    zmk_split_central_invoke_behavior(ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL, &binding, event,
                                      true);
    k_msleep(2);
    event.timestamp = k_uptime_get();
    zmk_split_central_invoke_behavior(ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL, &binding, event,
                                      false);

    return BRIDGE_RESPONSE_SIMPLE(true);
}

BRIDGE_SUBSYSTEM_HANDLER(underglow, set_ug_cmd);
