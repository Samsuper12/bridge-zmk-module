zephyr_include_directories(include)
zephyr_library()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)

nanopb_generate_cpp(bridge_proto_srcs bridge_proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/proto/core.proto
)

target_include_directories(app PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_sources(app PRIVATE ${bridge_proto_srcs} ${bridge_proto_hdrs})

target_sources(app PRIVATE src/bridge_uart.c)
target_sources(app PRIVATE src/events/bridge_event.c)

