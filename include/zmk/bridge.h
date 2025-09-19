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
#include <zmk/behavior.h>
#include <zephyr/sys/util.h>

typedef bridge_Response(bridge_func)(const bridge_Request *req);
typedef uint8_t(bridge_request_id)(const bridge_Request *req);

#define STR(x) #x
#define XSTR(x) STR(x)

struct bridge_subsystem_handler {
    bridge_func *func;
    // FIXME: create subsystems struct, autogen it, etc
    bridge_request_id *bridge_request_id;
    uint8_t subsystem_choice;
    uint8_t request_choice;
};

// TODO: add request_id to response.
#define BRIDGE_SUBSYSTEM_HANDLER(prefix, request_id)                                               \
    uint8_t subsystem_func_##request_id(const bridge_Request *req) {                               \
        return req->subsystem.prefix.which_request_type;                                           \
    }                                                                                              \
    bridge_Response exec_func_##request_id(const bridge_Request *req) {                            \
        LOG_INF("Calling Bridge handler: %s", XSTR(request_id));                                   \
        bridge_Response response = request_id(req);                                                \
        return response;                                                                           \
    }                                                                                              \
    STRUCT_SECTION_ITERABLE(bridge_subsystem_handler, prefix##_subsystem_handler_##request_id) = { \
        .func = exec_func_##request_id,                                                            \
        .bridge_request_id = subsystem_func_##request_id,                                          \
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

#define BRIDGE_NOTIFICATION(subsys, _type, ...)                                                    \
    ((bridge_Notification){                                                                        \
        .which_subsystem = bridge_Response_##subsys##_tag,                                         \
        .subsystem =                                                                               \
            {                                                                                      \
                .subsys =                                                                          \
                    {                                                                              \
                        .which_notification_type = bridge_##subsys##_Response_##_type##_tag,       \
                        .notification_type = {._type = __VA_ARGS__},                               \
                    },                                                                             \
            },                                                                                     \
    })

#define BRIDGE_RESPONSE_SIMPLE(status)                                                             \
    ((bridge_Response){                                                                            \
        .request_status = status,                                                                  \
    })

#define BRIDGE_DECLARE_BINDING(binding_var_name, event_var_name, node_name)                        \
    static const struct device *node_name##_dev = DEVICE_DT_GET(DT_NODELABEL(node_name));          \
    static struct zmk_behavior_binding binding_var_name = {                                        \
        .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(node_name)),                                   \
        .param1 = 0,                                                                               \
        .param2 = 0,                                                                               \
    };                                                                                             \
    static struct zmk_behavior_binding_event event_var_name = {                                    \
        .position = 0,                                                                             \
        .timestamp = 0,                                                                            \
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,                                          \
    };

static inline void bridge_tap_binding(const struct zmk_behavior_binding *src_binding,
                                      struct zmk_behavior_binding_event event) {
    event.timestamp = k_uptime_get();
    zmk_behavior_invoke_binding(src_binding, event, true);
    k_msleep(2);
    event.timestamp = k_uptime_get();
    zmk_behavior_invoke_binding(src_binding, event, false);
}