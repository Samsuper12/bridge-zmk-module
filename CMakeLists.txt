zephyr_include_directories(include)
zephyr_library()

zephyr_include_directories(${ZEPHYR_BASE}/modules/lib/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)

nanopb_generate_cpp(bridge_proto_srcs bridge_proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/proto/core.proto
)

zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})
zephyr_library_sources(${bridge_proto_srcs} ${bridge_proto_hdrs})

zephyr_library_sources(
    ${bridge_proto_srcs}
    ${bridge_proto_hdrs}
    src/bridge_uart.c
    src/events/bridge_event.c
)

