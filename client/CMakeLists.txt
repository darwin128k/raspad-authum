# Authum client library: every function a module calls on the Authum API
# (host getters, cfg, path, cstr, logger, sandbox, array, events, sql, json, …).
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

set(AUTHUM_SQLITE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/sqlite")
set(AUTHUM_CJSON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/cjson")
set(AUTHUM_MONOCYPHER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/monocypher")
if (NOT EXISTS "${AUTHUM_SQLITE_DIR}/sqlite3.c")
	message(FATAL_ERROR "Authum SQLite amalgamation missing at ${AUTHUM_SQLITE_DIR}")
endif ()
if (NOT EXISTS "${AUTHUM_CJSON_DIR}/cJSON.c")
	message(FATAL_ERROR "Authum cJSON missing at ${AUTHUM_CJSON_DIR}")
endif ()
if (NOT EXISTS "${AUTHUM_MONOCYPHER_DIR}/monocypher.c")
	message(FATAL_ERROR "Authum Monocypher missing at ${AUTHUM_MONOCYPHER_DIR}")
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

add_library(authum_cjson STATIC "${AUTHUM_CJSON_DIR}/cJSON.c")
target_include_directories(authum_cjson PUBLIC
	"$<BUILD_INTERFACE:${AUTHUM_CJSON_DIR}>"
)
set_target_properties(authum_cjson PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
)

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
	"${AUTHUM_SQLITE_DIR}"
	"${AUTHUM_CJSON_DIR}"
	"${AUTHUM_MONOCYPHER_DIR}"
)

target_link_libraries(authum_client PUBLIC authum_sqlite authum_cjson authum_monocypher)
# MySQL backend uses dlopen/LoadLibrary — need libdl on Unix.
if (NOT WIN32)
	target_link_libraries(authum_client PUBLIC ${CMAKE_DL_LIBS})
endif ()

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
	target_compile_options(authum_cjson PRIVATE -fvisibility=hidden
		-Wno-unused-parameter)
	target_compile_options(authum_monocypher PRIVATE -fvisibility=hidden -ffunction-sections
		-fdata-sections)
	if (AUTHUM_CLIENT_FORCE_32BIT)
		target_compile_options(authum_client PRIVATE -m32 -march=i686 -msse2)
		target_link_options(authum_client PRIVATE -m32)
		target_compile_options(authum_sqlite PRIVATE -m32 -march=i686 -msse2)
		target_link_options(authum_sqlite PRIVATE -m32)
		target_compile_options(authum_cjson PRIVATE -m32 -march=i686 -msse2)
		target_link_options(authum_cjson PRIVATE -m32)
		target_compile_options(authum_monocypher PRIVATE -m32 -march=i686 -msse2)
		target_link_options(authum_monocypher PRIVATE -m32)
	endif ()
endif ()
