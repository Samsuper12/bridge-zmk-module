zephyr_include_directories(include)
zephyr_include_directories(${ZEPHYR_BASE}/modules/nanopb)
zephyr_library()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)


zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY} 
    proto/core.proto
)

zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/../nanopb)


zephyr_library_sources(
    src/bridge_uart.c
    src/events/bridge_event.c
)

