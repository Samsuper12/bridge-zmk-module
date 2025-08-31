zephyr_include_directories(include)
zephyr_library()

list(APPEND CMAKE_MODULE_PATH modules/lib/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)

zephyr_include_directories(modules/lib/nanopb)

zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY} 
    proto/core.proto
)

zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})

zephyr_library_sources(
    src/bridge_uart.c
    src/events/bridge_event.c
)

