# Module API HTML (Doxygen) — AMXX /api analogue for Authum headers.
find_package(Doxygen)

if (NOT DOXYGEN_FOUND)
	message(STATUS "Doxygen not found — authum_api_docs target skipped")
	return ()
endif ()

find_program(DOT_EXECUTABLE dot)
if (DOT_EXECUTABLE)
	set(DOXYGEN_HAVE_DOT YES)
	get_filename_component(DOXYGEN_DOT_PATH "${DOT_EXECUTABLE}" DIRECTORY)
else ()
	set(DOXYGEN_HAVE_DOT NO)
	set(DOXYGEN_DOT_PATH "")
endif ()

# Prefer repo VERSION file when present; else CMake project version / 0.0.0.
set(_authum_ver_file "${CMAKE_SOURCE_DIR}/VERSION")
if (EXISTS "${_authum_ver_file}")
	file(READ "${_authum_ver_file}" PROJECT_VERSION)
	string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
elseif (DEFINED PROJECT_VERSION AND NOT PROJECT_VERSION STREQUAL "")
	# keep
else ()
	set(PROJECT_VERSION "0.0.0")
endif ()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
	"${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
	@ONLY
)

add_custom_target(authum_api_docs
	COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Generating Authum API docs (docs/api/html)"
	VERBATIM
)
