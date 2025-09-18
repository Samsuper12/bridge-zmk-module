zephyr_include_directories(include)
zephyr_include_directories(proto)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library_named(bridge-zmk-module)


if (CONFIG_ZMK_BRIDGE)

    set(BRIDGE_VERSION "1.0.0")
    set(BRIDGE_PROTO_SOURCE "")
    set(BRIDGE_PROTO_MODULE_NAMES "")

    message(STATUS "BRIDGE: Seaching for Modules")

    set(copied_proto_files "")
    set(bridge_main_proto "${ZEPHYR_CURRENT_MODULE_DIR}/proto/bridge.proto")

    file(GLOB_RECURSE module_proto_files
        "../*/proto/*.proto")

    foreach(module_proto ${module_proto_files})
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env python3 ${ZEPHYR_CURRENT_MODULE_DIR}/scripts/append_proto.py ${bridge_main_proto} ${module_proto}
            RESULT_VARIABLE script_result
            ERROR_VARIABLE script_error
        )

        if(NOT script_result EQUAL 0)
            message(FATAL_ERROR "BRIDGE: Failed to execute append_proto.py for ${module_proto}. Error: ${script_error}")
        endif()
        
        get_filename_component(proto_name ${module_proto} NAME)

        if (NOT "${proto_name}" STREQUAL "bridge.proto")
            configure_file(${module_proto} ${ZEPHYR_CURRENT_MODULE_DIR}/proto/${proto_name} COPYONLY)
            list(APPEND copied_proto_files ${ZEPHYR_CURRENT_MODULE_DIR}/proto/${proto_name})
        endif()
    endforeach()

    list(APPEND copied_proto_files "${bridge_main_proto}")
    foreach (file ${copied_proto_files})
        get_filename_component(proto_name ${file} NAME)
        set(BRIDGE_PROTO_SOURCE "${BRIDGE_PROTO_SOURCE}\\nfile:${proto_name}\\n")

        file(READ "${file}" PROTO_SOURCE_RAW)

        string(REPLACE "\\" "\\\\" PROTO_SOURCE "${PROTO_SOURCE_RAW}")
        string(REPLACE "\"" "\\\"" PROTO_SOURCE "${PROTO_SOURCE}")
        string(REPLACE "\n" "\\n" PROTO_SOURCE "${PROTO_SOURCE}")
        string(REPLACE ".proto" "" module_name "${proto_name}")
        
        if (BRIDGE_PROTO_MODULE_NAMES STREQUAL "")
            set(BRIDGE_PROTO_MODULE_NAMES "${module_name}")
        else()
            set(BRIDGE_PROTO_MODULE_NAMES "${BRIDGE_PROTO_MODULE_NAMES}, ${module_name}")
        endif()
        
        set(BRIDGE_PROTO_SOURCE "${BRIDGE_PROTO_SOURCE}${PROTO_SOURCE}")
    endforeach()

    configure_file(
        ${ZEPHYR_CURRENT_MODULE_DIR}/gen/bridge_gen.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/bridge_gen.h
        @ONLY
    )

    list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
    include(nanopb)
    set(NANOPB_GENERATE_CPP_STANDALONE OFF)

    zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY}
        ${copied_proto_files}
    )

    zephyr_include_directories(${ZEPHYR_NANOPB_MODULE_DIR})
    zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR}/proto)

    zephyr_linker_sources(SECTIONS include/linker/bridge_subsystem_handlers.ld)

    zephyr_library_sources(
        src/bridge_uart.c
        src/subsystems/core.c
        src/util/uart_framing.c
    )
endif()