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

# Embed lh into the plugin; skip its own tests/docs.
set(LH_BUILD_SHARED OFF CACHE BOOL "Build lh as a shared library" FORCE)
set(LH_BUILD_TESTS OFF CACHE BOOL "Build lh tests" FORCE)
set(LH_BUILD_DOCS OFF CACHE BOOL "Build lh documentation" FORCE)
set(LH_BUILD_BENCH OFF CACHE BOOL "Build lh benchmarks" FORCE)
set(LH_LIBRARY_OPTION_OS ON CACHE BOOL "lh OS backends (shared libraries, sockets)" FORCE)
# HLDS ships an old libgcc_s; bake the runtime into authum_mm.so.
set(LH_STATIC_LIBGCC ON CACHE BOOL "static-libgcc for HLDS" FORCE)
add_subdirectory(lib/lh)

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

target_link_libraries(authum PRIVATE
        lh
        metamod-p
)

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
