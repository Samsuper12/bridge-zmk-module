/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/bridge.h>
#include <pb_encode.h>
#include <zephyr/logging/log.h>

// TODO: rename to just bridge
LOG_MODULE_DECLARE(zmk_bridge, CONFIG_ZMK_LOG_LEVEL);

#define CORE_RESPONSE(type, ...) BRIDGE_RESPONSE(core, type, __VA_ARGS__)

static bool encode_device_info_name(pb_ostream_t *stream, const pb_field_t *field,
                                    void *const *arg) {
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    return pb_encode_string(stream, CONFIG_ZMK_KEYBOARD_NAME, strlen(CONFIG_ZMK_KEYBOARD_NAME));
}

#if IS_ENABLED(CONFIG_HWINFO)
static bool encode_device_info_serial_number(pb_ostream_t *stream, const pb_field_t *field,
                                             void *const *arg) {
    uint8_t id_buffer[32];
    const size_t id_size = hwinfo_get_device_id(id_buffer, ARRAY_SIZE(id_buffer));

    if (id_size <= 0) {
        return true;
    }

    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    return pb_encode_string(stream, id_buffer, id_size);
}

#endif // IS_ENABLED(CONFIG_HWINFO)

bridge_Response get_device_info(const bridge_Request *req) {
    bridge_core_GetDeviceInfoResponse resp = bridge_core_GetDeviceInfoResponse_init_zero;

    resp.name.funcs.encode = encode_device_info_name;
#if IS_ENABLED(CONFIG_HWINFO)
    resp.serial_number.funcs.encode = encode_device_info_serial_number;
#endif // IS_ENABLED(CONFIG_HWINFO)

    // TODO: Cmake help pls
    // resp.bridge_version.funcs.encode =

    return CORE_RESPONSE(get_device_info, resp);
}

BRIDGE_SUBSYSTEM_HANDLER(core, get_device_info);
