# Authum client library: every function a module calls on the Authum API
# (host getters, cfg, path, cstr, logger, sandbox, array, events, sql, json, …).
# Headers in include/ only declare; code lives in src/client/ and ships as
# this static library. Modules link `authum_client`; they never compile Authum
# `.c` files themselves. No Metamod. `authum_mm` links it too.
#
# JSON and SQL run on lh (lh/json, lh/sql) linked statically: no lh type
# reaches a module's headers, and --exclude-libs keeps lh's symbols out of the
# module's dynamic symbol table, so modules and the host never see each other's
# copy.

cmake_minimum_required(VERSION 3.12)

if (TARGET authum_client)
	return ()
endif ()

# HLDS modules are 32-bit; match when this dir is pulled in from a module build.
option(AUTHUM_CLIENT_FORCE_32BIT "Build authum_client as 32-bit" ON)

# A module project that pulls in only this directory gets lh from here too,
# configured the way <authum/sql.h> needs it.
if (NOT TARGET lh)
	set(AUTHUM_LH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/lh" CACHE PATH
		"lh source tree Authum builds with")
	set(LH_BUILD_TESTS OFF CACHE BOOL "Build lh tests" FORCE)
	set(LH_BUILD_DOCS OFF CACHE BOOL "Build lh documentation" FORCE)
	set(LH_BUILD_BENCH OFF CACHE BOOL "Build lh benchmarks" FORCE)
	set(LH_LIBRARY_OPTION_OS ON CACHE BOOL "lh OS backends (shared libraries)" FORCE)
	set(LH_LIBRARY_OPTION_SQL_SQLITE ON CACHE BOOL "SQLite in lh for <authum/sql.h>" FORCE)
	add_subdirectory("${AUTHUM_LH_DIR}" "${CMAKE_BINARY_DIR}/lh")
	if (AUTHUM_CLIENT_FORCE_32BIT AND NOT MSVC)
		target_compile_options(lh PRIVATE -m32 -march=i686 -msse2)
	endif ()
endif ()

file(GLOB_RECURSE AUTHUM_CLIENT_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/../src/client/*.c"
)
# src/client/os/<platform>/ is the only place that knows the OS: build the
# target's one and drop the other.
if (WIN32)
	list(FILTER AUTHUM_CLIENT_SOURCES EXCLUDE REGEX "/src/client/os/posix/")
else ()
	list(FILTER AUTHUM_CLIENT_SOURCES EXCLUDE REGEX "/src/client/os/win/")
endif ()

set(AUTHUM_MONOCYPHER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/monocypher")
if (NOT EXISTS "${AUTHUM_MONOCYPHER_DIR}/monocypher.c")
	message(FATAL_ERROR "Authum Monocypher missing at ${AUTHUM_MONOCYPHER_DIR}")
endif ()

# Monocypher 4.0.2 core + optional Ed25519 (RFC 8032) behind <authum/crypto/ed25519.h>.
add_library(authum_monocypher STATIC
	"${AUTHUM_MONOCYPHER_DIR}/monocypher.c"
	"${AUTHUM_MONOCYPHER_DIR}/monocypher-ed25519.c"
)
target_include_directories(authum_monocypher PUBLIC
	"$<BUILD_INTERFACE:${AUTHUM_MONOCYPHER_DIR}>"
)
set_target_properties(authum_monocypher PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
)

add_library(authum_client STATIC ${AUTHUM_CLIENT_SOURCES})

target_include_directories(authum_client
	PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
	PRIVATE
	"${AUTHUM_MONOCYPHER_DIR}"
)

target_link_libraries(authum_client PUBLIC authum_monocypher lh)
# The MySQL client is loaded with dlopen/LoadLibrary — libdl on Unix.
# iphlpapi lists the interface addresses for <authum/net.h> on Windows.
if (WIN32)
	target_link_libraries(authum_client PUBLIC iphlpapi)
else ()
	target_link_libraries(authum_client PUBLIC ${CMAKE_DL_LIBS})
endif ()
# lh is linked into every module: keep its symbols local to that module.
if (NOT MSVC AND NOT APPLE)
	target_link_options(authum_client INTERFACE "LINKER:--exclude-libs,liblh.a")
endif ()

set_target_properties(authum_client PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
)

if (NOT MSVC)
	target_compile_options(authum_client PRIVATE -fvisibility=hidden -ffunction-sections
		-fdata-sections)
	target_compile_options(authum_monocypher PRIVATE -fvisibility=hidden -ffunction-sections
		-fdata-sections)
	if (AUTHUM_CLIENT_FORCE_32BIT)
		target_compile_options(authum_client PRIVATE -m32 -march=i686 -msse2)
		target_link_options(authum_client PRIVATE -m32)
		target_compile_options(authum_monocypher PRIVATE -m32 -march=i686 -msse2)
		target_link_options(authum_monocypher PRIVATE -m32)
	endif ()
endif ()
