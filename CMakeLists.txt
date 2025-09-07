zephyr_include_directories(include)
zephyr_include_directories(proto)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library()

message(STATUS "BRIDGE: Seaching for Modules")

file(GLOB_RECURSE module_proto_files
    "${APPLICATION_SOURCE_DIR}/*/proto/*.proto")
message(STATUS "BRIDGE: modules ${ZEPHYR_MODULES}")


set(processed_proto_files "")
set(bridge_main_proto "${ZEPHYR_CURRENT_MODULE_DIR}/proto/bridge.proto")

foreach (module_path ${ZEPHYR_MODULES})
    file(GLOB_RECURSE module_proto_files
    "${module_path}/proto/*.proto")

    foreach(module_proto ${module_proto_files})
        message(STATUS "BRIDGE: Found module proto: ${module_proto}")
        execute_process(COMMAND python ${ZEPHYR_CURRENT_MODULE_DIR}/scripts/append_proto.py ${bridge_main_proto} ${module_proto})
        list(APPEND processed_proto_files ${module_proto})
    endforeach()
endforeach()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
set(NANOPB_GENERATE_CPP_STANDALONE OFF)

zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY} 
   ${ZEPHYR_CURRENT_MODULE_DIR}/proto/bridge.proto
   ${ZEPHYR_CURRENT_MODULE_DIR}/proto/core.proto
   ${processed_proto_files}
)

zephyr_include_directories(${ZEPHYR_NANOPB_MODULE_DIR})
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR}/proto)

zephyr_linker_sources(SECTIONS include/linker/bridge_subsystem_handlers.ld)

zephyr_library_sources(
    src/bridge_uart.c
    src/subsystems/core.c
    src/events/bridge_event.c
    src/util/uart_framing.c
)