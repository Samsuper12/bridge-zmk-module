/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <pb_encode.h>

#include <zmk/bridge.h>
#include <dt-bindings/zmk/rgb.h>
#include <zmk/rgb_underglow.h>
#include <zmk/behavior.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/virtual_key_position.h>

LOG_MODULE_DECLARE(zmk_bridge, CONFIG_ZMK_LOG_LEVEL);
BRIDGE_DECLARE_BINDING(binding, event, rgb_ug);

#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100
#define RGB_MAX 255

static struct zmk_led_hsb color_state = {.h = 0, .s = 0, .b = BRT_MAX};

static void invoke_hsb_cmd(const struct zmk_led_hsb state) {
    binding.param1 = RGB_COLOR_HSB_CMD;
    binding.param2 = RGB_COLOR_HSB_VAL(state.h, state.s, state.b);
    bridge_tap_binding(&binding, event);
}

static uint32_t c_clamp(uint32_t value, uint32_t min_val, uint32_t max_val) {
    if (value < min_val) {
        return min_val;
    } else if (value > max_val) {
        return max_val;
    } else {
        return value;
    }
}

static struct zmk_led_hsb rgb_to_hsb(uint32_t r, uint32_t g, uint32_t b) {
    struct zmk_led_hsb hsb_color;

    float r_n = (float)r / 255;
    float g_n = (float)g / 255;
    float b_n = (float)b / 255;

    float c_max = MAX(r_n, MAX(g_n, b_n));
    float delta = c_max - MIN(r_n, MIN(g_n, b_n));

    if (c_max == r_n) {
        hsb_color.h = (uint32_t)((60 * (g_n - b_n) / delta) + 360) % 360;
    } else if (c_max == g_n) {
        hsb_color.h = (uint32_t)((60 * (b_n - r_n) / delta) + 120) % 360;
    } else if (c_max == b_n) {
        hsb_color.h = (uint32_t)((60 * (r_n - g_n) / delta) + 240) % 360;
    }

    hsb_color.s = c_max == 0 ? 0 : (delta / c_max) * 100;
    hsb_color.b = c_max * 100;

    return hsb_color;
}

bridge_Response reset(const bridge_Request *req) {
    if (!device_is_ready(rgb_ug_dev)) {
        LOG_ERR("The rgb_ug node cannot be found!");
        return BRIDGE_RESPONSE_SIMPLE(false);
    }
    // White.
    color_state.h = 0;
    color_state.s = 0;
    color_state.b = BRT_MAX;
    invoke_hsb_cmd(color_state);

    return BRIDGE_RESPONSE_SIMPLE(true);
}

bridge_Response set_ug_cmd(const bridge_Request *req) {
    if (!device_is_ready(rgb_ug_dev)) {
        LOG_ERR("The rgb_ug node cannot be found!");
        return BRIDGE_RESPONSE_SIMPLE(false);
    }

    binding.param1 = req->subsystem.underglow.request_type.set_ug_cmd;
    binding.param2 = 0;
    bridge_tap_binding(&binding, event);

    return BRIDGE_RESPONSE_SIMPLE(true);
}

bridge_Response set_hsb(const bridge_Request *req) {
    if (!device_is_ready(rgb_ug_dev)) {
        LOG_ERR("The rgb_ug node cannot be found!");
        return BRIDGE_RESPONSE_SIMPLE(false);
    }

    bridge_underglow_Color_HSB color_hsb = req->subsystem.underglow.request_type.set_hsb;
    color_state.h = c_clamp(color_hsb.h, 0, HUE_MAX);
    color_state.s = c_clamp(color_hsb.s, 0, SAT_MAX);
    color_state.b = c_clamp(color_hsb.b, 0, BRT_MAX);
    invoke_hsb_cmd(color_state);

    return BRIDGE_RESPONSE_SIMPLE(true);
}

bridge_Response set_rgb(const bridge_Request *req) {
    if (!device_is_ready(rgb_ug_dev)) {
        LOG_ERR("The rgb_ug node cannot be found!");
        return BRIDGE_RESPONSE_SIMPLE(false);
    }

    bridge_underglow_Color_RGB color_rgb = req->subsystem.underglow.request_type.set_rgb;
    color_state = rgb_to_hsb(c_clamp(color_rgb.r, 0, RGB_MAX), c_clamp(color_rgb.g, 0, RGB_MAX),
                             c_clamp(color_rgb.b, 0, RGB_MAX));
    invoke_hsb_cmd(color_state);

    return BRIDGE_RESPONSE_SIMPLE(true);
}

bridge_Response set_brightness(const bridge_Request *req) {
    if (!device_is_ready(rgb_ug_dev)) {
        LOG_ERR("The rgb_ug node cannot be found!");
        return BRIDGE_RESPONSE_SIMPLE(false);
    }

    color_state.b = c_clamp(req->subsystem.underglow.request_type.set_brightness, 0, BRT_MAX);
    invoke_hsb_cmd(color_state);

    return BRIDGE_RESPONSE_SIMPLE(true);
}

bridge_Response set_saturation(const bridge_Request *req) {
    if (!device_is_ready(rgb_ug_dev)) {
        LOG_ERR("The rgb_ug node cannot be found!");
        return BRIDGE_RESPONSE_SIMPLE(false);
    }

    color_state.s = c_clamp(req->subsystem.underglow.request_type.set_saturation, 0, SAT_MAX);
    invoke_hsb_cmd(color_state);

    return BRIDGE_RESPONSE_SIMPLE(true);
}

bridge_Response set_hue(const bridge_Request *req) {
    if (!device_is_ready(rgb_ug_dev)) {
        LOG_ERR("The rgb_ug node cannot be found!");
        return BRIDGE_RESPONSE_SIMPLE(false);
    }

    color_state.h = c_clamp(req->subsystem.underglow.request_type.set_hue, 0, HUE_MAX);
    invoke_hsb_cmd(color_state);

    return BRIDGE_RESPONSE_SIMPLE(true);
}

BRIDGE_SUBSYSTEM_HANDLER(underglow, reset);
BRIDGE_SUBSYSTEM_HANDLER(underglow, set_ug_cmd);
BRIDGE_SUBSYSTEM_HANDLER(underglow, set_hsb);
BRIDGE_SUBSYSTEM_HANDLER(underglow, set_rgb);
BRIDGE_SUBSYSTEM_HANDLER(underglow, set_brightness);
BRIDGE_SUBSYSTEM_HANDLER(underglow, set_saturation);
BRIDGE_SUBSYSTEM_HANDLER(underglow, set_hue);
