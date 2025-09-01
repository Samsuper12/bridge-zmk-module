zephyr_include_directories(include)
zephyr_library()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)


#zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY} 
#    ${ZEPHYR_CURRENT_MODULE_DIR}/src/proto11111/core.proto
#)

zephyr_include_directories(${ZEPHYR_NANOPB_MODULE_DIR})

nanopb_generate_cpp(proto_bridge_srcs proto_bridge_hdrs RELPATH ${ZEPHYR_CURRENT_MODULE_DIR}/src/proto11111
${ZEPHYR_CURRENT_MODULE_DIR}/src/proto11111/core.proto
)

zephyr_library_sources(
    ${proto_bridge_srcs} ${proto_bridge_hdrs}
    src/bridge_uart.c
    src/events/bridge_event.c
)

