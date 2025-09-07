/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

// TODO: move this out of zmk folder
#pragma once

#include <bridge.pb.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/ring_buffer.h>

typedef bridge_Response(bridge_func)(const bridge_Request *req);

struct bridge_subsystem_handler {
    bridge_func *func;
    uint8_t subsystem_choice;
    uint8_t request_choice;
};

/**
 * @brief Register an RPC subsystem handler handler a specific request within the subsystem.
 * @param prefix The identifier for the subsystem, e.g. `core`, `keymap`, etc.
 * @param request_id The identifier for the request ID, e.g. `save_changes`.
 *
 * @note  A function with a name matching the request_id must be in-scope and will be used as the
 *        the callback handler. The function must have a signature of
 *        zmk_studio_Response (*func)(const zmk_studio_Request*)
 */
#define BRIDGE_SUBSYSTEM_HANDLER(prefix, request_id)                                               \
    STRUCT_SECTION_ITERABLE(bridge_subsystem_handler, prefix##_subsystem_handler_##request_id) = { \
        .func = request_id,                                                                        \
        .subsystem_choice = bridge_Request_##prefix##_tag,                                         \
        .request_choice = bridge_##prefix##_Request_##request_id##_tag,                            \
    };

#define BRIDGE_RESPONSE(subsys, _type, ...)                                                        \
    ((bridge_Response){                                                                            \
        .which_subsystem = bridge_Response_##subsys##_tag,                                         \
        .subsystem =                                                                               \
            {                                                                                      \
                .subsys =                                                                          \
                    {                                                                              \
                        .which_response_type = bridge_##subsys##_Response_##_type##_tag,           \
                        .response_type = {._type = __VA_ARGS__},                                   \
                    },                                                                             \
            },                                                                                     \
    })

#define BRIDGE_RESPONSE_SIMPLE(request_id, status)                                                 \
    ((bridge_Response){                                                                            \
        .request_id = request_id,                                                                  \
        .request_status = status,                                                                  \
    })

#define BRIDGE_RESPONSE_OK(request_id) BRIDGE_RESPONSE_SIMPLE(request_id, true)
#define BRIDGE_RESPONSE_FAIL(request_id) BRIDGE_RESPONSE_SIMPLE(request_id, false)