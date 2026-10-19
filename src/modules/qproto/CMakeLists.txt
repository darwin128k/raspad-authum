# Authum-packaged qproto (ABI 0.2). Core lives in this tree (no QPROTO_SRC_DIR).

set(QPROTO_METAMOD_P "${CMAKE_SOURCE_DIR}/lib/metamod-p")
if (NOT EXISTS "${QPROTO_METAMOD_P}/metamod/meta_api.h")
    message(WARNING "qproto: lib/metamod-p missing — Authum qproto module skipped (CI clones it)")
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
        core/qproto_hook.cpp
        core/qproto_engine.cpp
        core/qproto_cfg.cpp
        core/qproto_authorizers.cpp
        core/qproto_utils.cpp
        core/qproto_bans.cpp
        core/qproto_client_state.cpp
        core/qproto_api.cpp
        core/Rijndael.cpp
        core/sha2.cpp
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
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
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
            -ffunction-sections
            -fdata-sections
    )
    target_link_options(qproto_authum PRIVATE
            -static-libgcc
            -static-libstdc++
            "LINKER:--gc-sections"
    )
endif ()

target_link_libraries(qproto_authum PRIVATE dl m)

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    if (CMAKE_STRIP)
        add_custom_command(TARGET qproto_authum POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-unneeded "$<TARGET_FILE:qproto_authum>"
                COMMENT "strip qproto.so")
    endif ()
endif ()
