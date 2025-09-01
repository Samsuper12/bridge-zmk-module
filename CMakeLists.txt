zephyr_include_directories(include)
zephyr_include_directories(${ZEPHYR_BASE}/modules/nanopb)
zephyr_library()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)


nanopb_generate_cpp(proto_srcs proto_hdrs RELPATH proto
    proto/core.proto
)

zephyr_include_directories(${ZEPHYR_NANOPB_MODULE_DIR})

zephyr_library_sources(
    ${proto_srcs} 
    ${proto_hdrs}
    src/bridge_uart.c
    src/events/bridge_event.c
)

