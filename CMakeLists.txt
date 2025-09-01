zephyr_include_directories(include)
zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/modules/nanopb)
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})
zephyr_library()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_CURRENT_MODULE_DIR}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)


zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY} 
    proto/core.proto
)


zephyr_library_sources(
    src/bridge_uart.c
    src/events/bridge_event.c
)

