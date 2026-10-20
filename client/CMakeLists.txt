# Authum client helpers for modules — headers in include/, code here as a
# static library. Foreign modules link `authum_client`; they do not compile
# Authum `.c` files themselves. No Metamod, no lh.

cmake_minimum_required(VERSION 3.12)

if (TARGET authum_client)
    return ()
endif ()

add_library(authum_client STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/client/cfg.c"
)

target_include_directories(authum_client
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
)

set_target_properties(authum_client PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
)

# HLDS modules are 32-bit; match when this dir is pulled in from a module build.
option(AUTHUM_CLIENT_FORCE_32BIT "Build authum_client as 32-bit" ON)

if (NOT MSVC)
    target_compile_options(authum_client PRIVATE -fvisibility=hidden)
    if (AUTHUM_CLIENT_FORCE_32BIT)
        target_compile_options(authum_client PRIVATE -m32 -march=i686 -msse2)
        target_link_options(authum_client PRIVATE -m32)
    endif ()
endif ()
