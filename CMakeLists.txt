zephyr_include_directories(include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)

zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY} 
   ${ZEPHYR_CURRENT_MODULE_DIR}/proto/bridge.proto
   ${ZEPHYR_CURRENT_MODULE_DIR}/proto/core.proto
)

zephyr_include_directories(${ZEPHYR_NANOPB_MODULE_DIR})


zephyr_linker_sources(SECTIONS include/linker/bridge_subsystem_handlers.ld)

zephyr_library_sources(
    src/bridge_uart.c
    src/subsystems/core.c
    src/events/bridge_event.c
    src/util/uart_framing.c
)