/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <proto/bridge.pb.h>

typedef bridge_Response(bridge_func)(const bridge_Request *req);

struct bridge_subsystem_handler {
    bridge_func *func;
    uint8_t subsystem_choice;
    uint8_t request_choice;
};

static inline struct bridge_subsystem_handler *find_subsystem_handler_for_choice(uint8_t choice) {
    STRUCT_SECTION_FOREACH(bridge_subsystem_handler, sub) {
        if (sub->subsystem_choice == choice) {
            return sub;
        }
    }

    return NULL;
}

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
        .which_subsystem = bridge_##subsys##_Response_tag,                                         \
        .subsystem =                                                                               \
            {                                                                                      \
                .subsys =                                                                          \
                    {                                                                              \
                        .which_response_type = bridge_##subsys##_Response_##_type##_tag,           \
                        .response_type = {._type = __VA_ARGS__},                                   \
                    },                                                                             \
            },                                                                                     \
    })
