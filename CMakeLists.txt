cmake_minimum_required(VERSION 3.12)
project(authum
        DESCRIPTION "Metamod-P plugin for original HLDS"
        LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GoldSrc / original HLDS loads 32-bit modules.
option(AUTHUM_FORCE_32BIT "Build for 32-bit HLDS" ON)

set(AUTHUM_HLSDK_DIR "${CMAKE_SOURCE_DIR}/lib/metamod-p/hlsdk" CACHE PATH
        "Half-Life SDK root (engine/, common/, dlls/, pm_shared/)")

if (AUTHUM_FORCE_32BIT)
    if (MSVC)
        if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
            message(FATAL_ERROR
                    "HLDS plugins must be 32-bit. Use a Win32 generator (e.g. -A Win32).")
        endif ()
    else ()
        add_compile_options(-m32 -march=i686 -msse2)
        add_link_options(-m32)
    endif ()
endif ()

# Authum is locked to lh 0.4.0 (INI + LH_STATIC_LIBGCC). Clone with
# --recurse-submodules; .gitmodules tracks branch 0.4.0. Refuse any other VERSION.
set(AUTHUM_LH_REQUIRED_VERSION "0.4.0")
set(_authum_lh_version_file "${CMAKE_SOURCE_DIR}/lib/lh/VERSION")
if (NOT EXISTS "${_authum_lh_version_file}")
    message(FATAL_ERROR
            "lib/lh is missing (no VERSION). Run:\n"
            "  git submodule update --init --recursive\n"
            "Authum requires lh ${AUTHUM_LH_REQUIRED_VERSION} (branch 0.4.0).")
endif ()
file(READ "${_authum_lh_version_file}" _authum_lh_version)
string(STRIP "${_authum_lh_version}" _authum_lh_version)
if (NOT _authum_lh_version STREQUAL AUTHUM_LH_REQUIRED_VERSION)
    message(FATAL_ERROR
            "lib/lh VERSION is '${_authum_lh_version}', Authum requires "
            "${AUTHUM_LH_REQUIRED_VERSION} (git branch 0.4.0).\n"
            "Fix with:\n"
            "  git -C lib/lh fetch origin\n"
            "  git -C lib/lh checkout 0.4.0\n"
            "  git submodule update --init lib/lh")
endif ()

# Embed lh into the plugin; skip its own tests/docs.
#
# AUTHUM_LH_LINK_MODE — how dependents pull static lh (lh has no shared build):
#   needed  (default) — liblh.a, only referenced .o (+ --gc-sections)
#   whole             — -Wl,--whole-archive liblh.a (every lh object)
set(AUTHUM_LH_LINK_MODE "needed" CACHE STRING
        "How Authum targets link lh: needed | whole")
set_property(CACHE AUTHUM_LH_LINK_MODE PROPERTY STRINGS needed whole)

set(LH_BUILD_SHARED OFF CACHE BOOL "Build lh as a shared library" FORCE)
set(LH_BUILD_TESTS OFF CACHE BOOL "Build lh tests" FORCE)
set(LH_BUILD_DOCS OFF CACHE BOOL "Build lh documentation" FORCE)
set(LH_BUILD_BENCH OFF CACHE BOOL "Build lh benchmarks" FORCE)
set(LH_LIBRARY_OPTION_OS ON CACHE BOOL "lh OS backends (shared libraries, sockets)" FORCE)
# HLDS ships an old libgcc_s; bake the runtime into authum_mm.so.
set(LH_STATIC_LIBGCC ON CACHE BOOL "static-libgcc for HLDS" FORCE)
add_subdirectory(lib/lh)

# Link lh per AUTHUM_LH_LINK_MODE, then gc-sections + strip on Release.
function(authum_link_lh target)
    if (NOT TARGET ${target})
        return()
    endif ()
    if (AUTHUM_LH_LINK_MODE STREQUAL "whole" AND NOT MSVC)
        message(STATUS "${target}: AUTHUM_LH_LINK_MODE=whole (--whole-archive liblh.a)")
        target_link_libraries(${target} PRIVATE
                "-Wl,--whole-archive" lh "-Wl,--no-whole-archive")
    else ()
        message(STATUS "${target}: AUTHUM_LH_LINK_MODE=needed (static, referenced .o only)")
        target_link_libraries(${target} PRIVATE lh)
    endif ()
    if (NOT MSVC)
        target_compile_options(${target} PRIVATE -ffunction-sections -fdata-sections)
        target_link_options(${target} PRIVATE "LINKER:--gc-sections")
    endif ()
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        if (CMAKE_STRIP)
            add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_STRIP} --strip-unneeded "$<TARGET_FILE:${target}>"
                    COMMENT "strip $<TARGET_FILE_NAME:${target}>")
        endif ()
    endif ()
endfunction()

add_library(metamod-p INTERFACE)
target_include_directories(metamod-p INTERFACE
        "${CMAKE_SOURCE_DIR}/lib/metamod-p/metamod"
)

if (EXISTS "${AUTHUM_HLSDK_DIR}/engine" AND EXISTS "${AUTHUM_HLSDK_DIR}/common")
    target_include_directories(metamod-p INTERFACE
            "${AUTHUM_HLSDK_DIR}"
            "${AUTHUM_HLSDK_DIR}/engine"
            "${AUTHUM_HLSDK_DIR}/common"
            "${AUTHUM_HLSDK_DIR}/dlls"
            "${AUTHUM_HLSDK_DIR}/pm_shared"
    )
else ()
    message(WARNING
            "HLSDK not found at AUTHUM_HLSDK_DIR=${AUTHUM_HLSDK_DIR}. "
            "Point this cache variable at an HLSDK tree before compiling plugin sources.")
endif ()

file(GLOB_RECURSE AUTHUM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/authum/*.c"
        "${CMAKE_SOURCE_DIR}/src/authum/*.cpp"
)
file(GLOB_RECURSE AUTHUM_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/include/authum/*.h"
        "${CMAKE_SOURCE_DIR}/include/authum/*.hpp"
)

if (NOT AUTHUM_SOURCES)
    message(STATUS "No sources under src/authum yet; plugin target is skipped.")
    return ()
endif ()

add_library(authum MODULE ${AUTHUM_SOURCES} ${AUTHUM_HEADERS})
set_target_properties(authum PROPERTIES
        PREFIX ""
        OUTPUT_NAME "authum_mm"
        CXX_EXTENSIONS ON
)

target_include_directories(authum PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(authum PRIVATE metamod-p)
authum_link_lh(authum)

if (WIN32)
    target_compile_definitions(authum PRIVATE WIN32 _WINDOWS)
else ()
    target_compile_definitions(authum PRIVATE linux LINUX)
    target_link_libraries(authum PRIVATE m)
    if (NOT MSVC)
        target_compile_options(authum PRIVATE
                -fno-exceptions
                -fno-rtti
                -fvisibility=hidden
        )
    endif ()
endif ()

# Child modules: examples always optional; qproto if QPROTO_SRC_DIR is found.
option(AUTHUM_BUILD_EXAMPLES "Build example Authum modules under examples/" ON)
if (AUTHUM_BUILD_EXAMPLES)
    add_subdirectory(examples/hello)
endif ()

option(AUTHUM_BUILD_QPROTO "Build Authum-packaged qproto module" ON)
if (AUTHUM_BUILD_QPROTO)
    add_subdirectory(src/modules/qproto)
endif ()
