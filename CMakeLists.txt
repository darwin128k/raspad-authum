cmake_minimum_required(VERSION 3.12)
project(authum
        DESCRIPTION "Authum as the HLDS game library"
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

set(AUTHUM_HLSDK_DIR "${CMAKE_SOURCE_DIR}/lib/hlsdk" CACHE PATH
        "Half-Life SDK root (engine/, common/, dlls/, pm_shared/)")

if (AUTHUM_FORCE_32BIT)
    if (MSVC)
        if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
            message(FATAL_ERROR
                    "HLDS plugins must be 32-bit. Use a Win32 generator (e.g. -A Win32).")
        endif ()
    else ()
        # A 64-bit-only toolchain (MSYS2 mingw64, Linux without multilib) takes
        # -m32 when compiling and only fails at the first link. Find out now.
        include(CheckCSourceCompiles)
        set(CMAKE_REQUIRED_FLAGS "-m32")
        check_c_source_compiles("int main(void) { return sizeof(void *) == 4 ? 0 : 1; }"
                AUTHUM_CAN_LINK_M32)
        unset(CMAKE_REQUIRED_FLAGS)
        if (NOT AUTHUM_CAN_LINK_M32)
            if (MINGW)
                set(_authum_m32_hint
                        "This MinGW (${CMAKE_C_COMPILER}) has no 32-bit libraries. Use the "
                        "32-bit one: cmake --preset windows-mingw32 (MSYS2 C:/msys64/mingw32, "
                        "pacman -S mingw-w64-i686-gcc mingw-w64-i686-ninja).")
            else ()
                set(_authum_m32_hint
                        "Install 32-bit libraries, e.g. on Debian/Ubuntu: "
                        "apt install gcc-multilib g++-multilib.")
            endif ()
            message(FATAL_ERROR
                    "HLDS plugins must be 32-bit, but ${CMAKE_C_COMPILER} cannot link with -m32.\n"
                    ${_authum_m32_hint}
                    "\nDelete the build directory before configuring again.")
        endif ()
        add_compile_options(-m32 -march=i686 -msse2)
        add_link_options(-m32)
        # 64-bit file offsets and inode numbers in Authum's own code (stat,
        # lseek): a 32-bit build otherwise gets EOVERFLOW on XFS / btrfs /
        # network mounts. lh sets the same for itself.
        if (NOT WIN32)
            add_compile_definitions(_FILE_OFFSET_BITS=64)
        endif ()
    endif ()
endif ()

# Authum is locked to lh 0.4.0 (INI, JSON, SQL + LH_STATIC_LIBGCC). Clone with
# --recurse-submodules; .gitmodules tracks branch 0.4.0. Refuse any other VERSION.
#
# AUTHUM_LH_DIR points the build at another lh tree (an lh checkout being worked
# on alongside Authum); it defaults to the submodule.
set(AUTHUM_LH_DIR "${CMAKE_SOURCE_DIR}/lib/lh" CACHE PATH "lh source tree Authum builds with")
set(AUTHUM_LH_REQUIRED_VERSION "0.4.0")
set(_authum_lh_version_file "${AUTHUM_LH_DIR}/VERSION")
if (NOT EXISTS "${_authum_lh_version_file}")
    message(FATAL_ERROR
            "${AUTHUM_LH_DIR} is missing (no VERSION). Run:\n"
            "  git submodule update --init --recursive\n"
            "Authum requires lh ${AUTHUM_LH_REQUIRED_VERSION} (branch 0.4.0).")
endif ()
file(READ "${_authum_lh_version_file}" _authum_lh_version)
string(STRIP "${_authum_lh_version}" _authum_lh_version)
if (NOT _authum_lh_version STREQUAL AUTHUM_LH_REQUIRED_VERSION)
    message(FATAL_ERROR
            "${AUTHUM_LH_DIR} VERSION is '${_authum_lh_version}', Authum requires "
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
# <authum/sql.h> runs on lh/sql: SQLite compiled into lh, MySQL loaded at run time.
set(LH_LIBRARY_OPTION_SQL_SQLITE ON CACHE BOOL "SQLite in lh for <authum/sql.h>" FORCE)
# HLDS ships an old libgcc_s; bake the runtime into authum.so.
set(LH_STATIC_LIBGCC ON CACHE BOOL "static-libgcc for HLDS" FORCE)
add_subdirectory("${AUTHUM_LH_DIR}" "${CMAKE_BINARY_DIR}/lh")

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

# The HLSDK headers (engine/, common/, dlls/, pm_shared/). Authum is the game
# library itself: Metamod is not part of the build, only Valve's SDK headers
# are — types and macros, no linkage.
add_library(hlsdk INTERFACE)
if (EXISTS "${AUTHUM_HLSDK_DIR}/engine" AND EXISTS "${AUTHUM_HLSDK_DIR}/common")
    target_include_directories(hlsdk INTERFACE
            "${AUTHUM_HLSDK_DIR}"
            "${AUTHUM_HLSDK_DIR}/engine"
            "${AUTHUM_HLSDK_DIR}/common"
            "${AUTHUM_HLSDK_DIR}/dlls"
            "${AUTHUM_HLSDK_DIR}/pm_shared"
    )
else ()
    message(WARNING
            "HLSDK not found at AUTHUM_HLSDK_DIR=${AUTHUM_HLSDK_DIR}. "
            "Point this cache variable at an HLSDK tree before compiling.")
endif ()

# Host: the engine-independent core (loader, events, gate, log, sandbox, …).
file(GLOB_RECURSE AUTHUM_HOST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/authum/host/*.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/*.cpp"
)
# The entry layer: Authum as the HLDS game library (src/authum/gamedll/).
file(GLOB AUTHUM_GAMEDLL_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/authum/gamedll/*.cpp"
)
file(GLOB_RECURSE AUTHUM_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/include/authum/*.h"
        "${CMAKE_SOURCE_DIR}/include/authum/*.hpp"
)

if (NOT AUTHUM_HOST_SOURCES)
    message(STATUS "No sources under src/authum/host yet; the target is skipped.")
    return ()
endif ()

# Module API (src/authum/api: cfg, path, host getters, sandbox, events, …) — the static lib modules link.
add_subdirectory(client)

# Settings common to every Authum image.
function (authum_configure_image target)
    set_target_properties(${target} PROPERTIES PREFIX "" CXX_EXTENSIONS ON)
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/include")
    target_link_libraries(${target} PRIVATE hlsdk authum_client)
    authum_link_lh(${target})
    if (WIN32)
        target_compile_definitions(${target} PRIVATE WIN32 _WINDOWS)
    else ()
        target_compile_definitions(${target} PRIVATE linux LINUX)
        target_link_libraries(${target} PRIVATE m ${CMAKE_DL_LIBS})
        if (NOT MSVC)
            target_compile_options(${target} PRIVATE
                    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
                    -fvisibility=hidden
            )
        endif ()
    endif ()
endfunction ()

# The game library Authum wraps (Counter-Strike: cs.so).
#
# The engine looks an entity's spawn function up by class name in the game
# module — the one it loaded, which is us. So we export the class names
# ourselves (src/authum/gamedll/game_classes.cpp, generated by
# tools/gen-game-classes.py) and each trampoline forwards to the game. That
# works the same on Linux and on Windows — no dependency trick, no
# RTLD_GLOBAL, nothing platform-specific.
set(AUTHUM_GAME_DLL_NAME "cs.so" CACHE STRING
        "File name of the game library Authum wraps (cs.so for Counter-Strike)")

# authum: Authum as the HLDS game library (the image the engine loads).
if (AUTHUM_GAMEDLL_SOURCES)
    add_library(authum MODULE
            ${AUTHUM_HOST_SOURCES} ${AUTHUM_GAMEDLL_SOURCES} ${AUTHUM_HEADERS})
    set_target_properties(authum PROPERTIES OUTPUT_NAME "authum")
    authum_configure_image(authum)
    if (NOT WIN32)
        set_target_properties(authum PROPERTIES
                BUILD_RPATH "$ORIGIN/../../dlls;$ORIGIN/../.."
                INSTALL_RPATH "$ORIGIN/../../dlls;$ORIGIN/../..")
    elseif (MINGW)
        # GiveFnptrsToDll is __stdcall: export it as the engine asks for it,
        # without the @8 suffix.
        target_link_options(authum PRIVATE -Wl,--kill-at)
        # The Windows engine reads the game DLL's export names itself (for
        # save/restore) and takes them from the section named ".rdata" — where
        # MSVC puts the export table. GNU ld keeps it in ".edata", so the engine
        # reads garbage and faults. Only the names change: the loader ignores
        # them, the layout and the export directory stay as linked.
        add_custom_command(TARGET authum POST_BUILD
                COMMAND ${CMAKE_OBJCOPY}
                        --rename-section .rdata=.rodata
                        --rename-section .edata=.rdata
                        "$<TARGET_FILE:authum>"
                COMMENT "authum.dll: export table into .rdata (the engine looks there)")
    endif ()
else ()
    message(STATUS
            "authum (the game library) is Linux-only — this build makes "
            "authum_client for modules to link against.")
endif ()

# Authum builds authum.so + authum_client (+ optional examples). Real modules
# are separate projects that link authum_client and drop into modules/ at runtime.
option(AUTHUM_BUILD_EXAMPLES "Build example Authum modules under examples/" ON)
if (AUTHUM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

option(AUTHUM_BUILD_TESTS "Build Authum tests (ctest)" ON)
if (AUTHUM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

# Browsable module API (Doxygen → docs/api/html), like amxmodx.org/api.
option(AUTHUM_BUILD_API_DOCS "Generate Authum API HTML from include/authum" ON)
if (AUTHUM_BUILD_API_DOCS)
    add_subdirectory(docs/api)
endif ()
