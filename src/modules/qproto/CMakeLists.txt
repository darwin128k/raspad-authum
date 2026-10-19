# Authum-packaged qproto (ABI 0.2 client). Core sources from QPROTO_SRC_DIR.

set(QPROTO_SRC_DIR "" CACHE PATH
        "Checkout of raspad-qproto (has src/qproto and include/qproto)")

# Auto-detect when unset or still pointing at a missing tree.
if (QPROTO_SRC_DIR STREQUAL "" OR NOT EXISTS "${QPROTO_SRC_DIR}/src/qproto/qproto_hook.cpp")
    foreach (_cand IN ITEMS
            "${CMAKE_SOURCE_DIR}/../raspad-qproto"
            "${CMAKE_SOURCE_DIR}/../vellum/qproto"
            "/mnt/h/Games/Counter-Strike 1.6/vellum/qproto"
            "H:/Games/Counter-Strike 1.6/vellum/qproto")
        if (EXISTS "${_cand}/src/qproto/qproto_hook.cpp")
            set(QPROTO_SRC_DIR "${_cand}" CACHE PATH
                    "Checkout of raspad-qproto (has src/qproto and include/qproto)" FORCE)
            break()
        endif ()
    endforeach ()
endif ()

if (NOT EXISTS "${QPROTO_SRC_DIR}/src/qproto/qproto_hook.cpp")
    message(STATUS "qproto: QPROTO_SRC_DIR not found — Authum qproto module skipped")
    return()
endif ()

message(STATUS "qproto: sources from ${QPROTO_SRC_DIR}")

set(QPROTO_METAMOD_P "${QPROTO_SRC_DIR}/lib/metamod-p")
if (NOT EXISTS "${QPROTO_METAMOD_P}/metamod/meta_api.h")
    # Fall back to Authum's local metamod-p if present.
    set(QPROTO_METAMOD_P "${CMAKE_SOURCE_DIR}/lib/metamod-p")
endif ()
if (NOT EXISTS "${QPROTO_METAMOD_P}/metamod/meta_api.h")
    message(WARNING "qproto: metamod-p headers missing — Authum qproto module skipped")
    return()
endif ()

set(QPROTO_DEP_METAMOD "${QPROTO_METAMOD_P}/metamod")
set(QPROTO_DEP_HLSDK "${QPROTO_METAMOD_P}/hlsdk")

add_library(qproto_authum MODULE
        info.c
        module.c
        bridge.cpp
        host_log.cpp
        log.cpp
        paths.cpp
        data_dir.cpp
        engfuncs_shim.cpp
        "${QPROTO_SRC_DIR}/src/qproto/qproto_hook.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/qproto_engine.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/qproto_cfg.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/qproto_authorizers.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/qproto_utils.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/qproto_bans.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/qproto_client_state.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/qproto_api.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/Rijndael.cpp"
        "${QPROTO_SRC_DIR}/src/qproto/sha2.cpp"
        "${QPROTO_DEP_METAMOD}/sdk_util.cpp"
)

set_target_properties(qproto_authum PROPERTIES
        PREFIX ""
        OUTPUT_NAME "qproto"
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS ON
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules/qproto"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules/qproto"
)

target_include_directories(qproto_authum PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${QPROTO_SRC_DIR}/include"
        "${QPROTO_DEP_METAMOD}"
        "${QPROTO_DEP_HLSDK}/dlls"
        "${QPROTO_DEP_HLSDK}/engine"
        "${QPROTO_DEP_HLSDK}/common"
        "${QPROTO_DEP_HLSDK}/pm_shared"
)

target_compile_definitions(qproto_authum PRIVATE
        linux LINUX
)

if (NOT MSVC)
    target_compile_options(qproto_authum PRIVATE
            -fpermissive
            -fvisibility=hidden
            -Wall
    )
    target_link_options(qproto_authum PRIVATE
            -static-libgcc
            -static-libstdc++
            "LINKER:--gc-sections"
    )
    target_compile_options(qproto_authum PRIVATE -ffunction-sections -fdata-sections)
endif ()

target_link_libraries(qproto_authum PRIVATE dl m)

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    if (CMAKE_STRIP)
        add_custom_command(TARGET qproto_authum POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-unneeded "$<TARGET_FILE:qproto_authum>"
                COMMENT "strip qproto.so")
    endif ()
endif ()
