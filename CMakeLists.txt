zephyr_include_directories(include)
zephyr_library()

zephyr_library_sources(src/bridge_uart.c)
zephyr_library_sources(src/events/bridge_event.c)

