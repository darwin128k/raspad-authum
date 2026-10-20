# Authum client library: every function a module calls on the Authum API
# (host getters, cfg, path, cstr, logger, sandbox, array, events, sql, …).
# Headers in include/ only declare; code lives in src/client/ and ships as
# this static library. Modules link `authum_client`; they never compile Authum
# `.c` files themselves. No Metamod, no lh. `authum_mm` links it too.

cmake_minimum_required(VERSION 3.12)

if (TARGET authum_client)
	return ()
endif ()

file(GLOB_RECURSE AUTHUM_CLIENT_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/../src/client/*.c"
)

set(AUTHUM_SQLITE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/sqlite")
if (NOT EXISTS "${AUTHUM_SQLITE_DIR}/sqlite3.c")
	message(FATAL_ERROR "Authum SQLite amalgamation missing at ${AUTHUM_SQLITE_DIR}")
endif ()

add_library(authum_sqlite STATIC "${AUTHUM_SQLITE_DIR}/sqlite3.c")
target_include_directories(authum_sqlite PUBLIC
	"$<BUILD_INTERFACE:${AUTHUM_SQLITE_DIR}>"
)
set_target_properties(authum_sqlite PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
)
target_compile_definitions(authum_sqlite PRIVATE
	SQLITE_OMIT_LOAD_EXTENSION
	SQLITE_THREADSAFE=1
)

add_library(authum_client STATIC ${AUTHUM_CLIENT_SOURCES})

target_include_directories(authum_client
	PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
	PRIVATE
	"${AUTHUM_SQLITE_DIR}"
)

target_link_libraries(authum_client PUBLIC authum_sqlite)

set_target_properties(authum_client PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
)

# HLDS modules are 32-bit; match when this dir is pulled in from a module build.
option(AUTHUM_CLIENT_FORCE_32BIT "Build authum_client as 32-bit" ON)

if (NOT MSVC)
	target_compile_options(authum_client PRIVATE -fvisibility=hidden -ffunction-sections
		-fdata-sections)
	target_compile_options(authum_sqlite PRIVATE -fvisibility=hidden
		-Wno-unused-parameter -Wno-sign-compare)
	if (AUTHUM_CLIENT_FORCE_32BIT)
		target_compile_options(authum_client PRIVATE -m32 -march=i686 -msse2)
		target_link_options(authum_client PRIVATE -m32)
		target_compile_options(authum_sqlite PRIVATE -m32 -march=i686 -msse2)
		target_link_options(authum_sqlite PRIVATE -m32)
	endif ()
endif ()
