zephyr_library()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)

zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY} 
   ${ZEPHYR_CURRENT_MODULE_DIR}/proto/core.proto
)

zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)
zephyr_include_directories(${ZEPHYR_NANOPB_MODULE_DIR})

zephyr_library_sources(
    src/bridge_uart.c
    src/events/bridge_event.c
)