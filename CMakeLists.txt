zephyr_include_directories(include)
zephyr_library()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)

nanopb_generate_cpp(bridge_proto_srcs bridge_proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/proto/core.proto
)

zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})
zephyr_library_sources(${bridge_proto_srcs})

zephyr_library_sources(src/bridge_uart.c)
zephyr_library_sources(src/events/bridge_event.c)

