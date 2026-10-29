# Authum suites share the existing pinned GoogleTest dependency and common runner.
if (NOT TARGET gtest)
    if (NOT EXISTS "${AUTHUM_LH_DIR}/test/lib/googletest/CMakeLists.txt")
        message(FATAL_ERROR "Initialize LH's pinned test/lib/googletest submodule to build tests")
    endif ()
    if (MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif ()
    add_subdirectory("${AUTHUM_LH_DIR}/test/lib/googletest" "${CMAKE_BINARY_DIR}/googletest"
            EXCLUDE_FROM_ALL)
endif ()

add_library(authum_test_main STATIC "${AUTHUM_LH_DIR}/test/src/main.cpp")
target_compile_features(authum_test_main PUBLIC cxx_std_20)
target_link_libraries(authum_test_main PUBLIC gtest)

# Authum GoogleTest suites run by CTest. Host pieces (sandbox, events,
# commands, logs) are compiled from src/authum/host together with lh, exactly as
# authum uses them.

add_executable(authum_test_client
        src/authum/cstr.cpp
        src/authum/path.cpp
        src/authum/cfg.cpp
        src/authum/engine.cpp
        src/authum/event/message.cpp
        src/authum/env.cpp
        src/authum/host.cpp
        src/authum/version.cpp
        src/authum/crypto/ed25519.cpp
        src/authum/detour.cpp
        src/authum/symbol.cpp
        src/authum/net.cpp
        src/authum/json.cpp
)
target_link_libraries(authum_test_client PRIVATE authum_client authum_test_main)
add_test(NAME authum_client COMMAND authum_test_client)

add_library(authum_test_host STATIC
        "${CMAKE_SOURCE_DIR}/src/authum/host/sandbox.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/events.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/events/ops.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/commands.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/logs.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/tasks.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/threads.c"
)
target_include_directories(authum_test_host PUBLIC "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(authum_test_host PUBLIC lh authum_client)

add_executable(authum_test_sandbox src/authum/host/sandbox.cpp)
target_link_libraries(authum_test_sandbox PRIVATE authum_test_host authum_test_main)
add_test(NAME authum_sandbox COMMAND authum_test_sandbox)

add_executable(authum_test_events src/authum/host/events.cpp)
target_link_libraries(authum_test_events PRIVATE authum_test_host authum_test_main)
add_test(NAME authum_events COMMAND authum_test_events)

add_executable(authum_test_commands src/authum/host/commands.cpp)
target_link_libraries(authum_test_commands PRIVATE authum_test_host authum_test_main)
add_test(NAME authum_commands COMMAND authum_test_commands)

add_executable(authum_test_timers src/authum/timer.cpp)
target_link_libraries(authum_test_timers PRIVATE authum_test_host authum_test_main)
add_test(NAME authum_timers COMMAND authum_test_timers)

add_executable(authum_test_logs src/authum/host/logs.cpp)
target_link_libraries(authum_test_logs PRIVATE authum_test_host authum_test_main)
add_test(NAME authum_logs COMMAND authum_test_logs --gtest_filter=authum_logs.*)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/logs_environment")
add_test(NAME authum_logs_environment COMMAND authum_test_logs --gtest_filter=authum_logs_environment.*)
set_tests_properties(authum_logs_environment PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/logs_environment"
        ENVIRONMENT "AUTHUM_LOG_LEVEL=off")

add_executable(authum_test_tasks src/authum/host/tasks.cpp)
target_link_libraries(authum_test_tasks PRIVATE authum_test_host authum_test_main)
add_test(NAME authum_tasks COMMAND authum_test_tasks)
set_tests_properties(authum_tasks PROPERTIES TIMEOUT 20)

add_executable(authum_test_common
        src/authum/logger/buffer.cpp
        src/authum/client/ref.cpp
        src/authum/sql.cpp
        src/authum/net/address.cpp
)
target_link_libraries(authum_test_common PRIVATE authum_test_host authum_test_main)
add_test(NAME authum_common COMMAND authum_test_common)
set_tests_properties(authum_common PROPERTIES TIMEOUT 20)

# Smoke test of examples/: loads every built example module.
if (AUTHUM_BUILD_EXAMPLES)
    add_executable(authum_test_examples examples.cpp)
    target_link_libraries(authum_test_examples PRIVATE authum_test_host authum_test_main
            ${CMAKE_DL_LIBS})
    target_compile_definitions(authum_test_examples PRIVATE
            AUTHUM_TEST_EXAMPLES_DIR="${CMAKE_BINARY_DIR}/examples"
            AUTHUM_TEST_MODULE_SUFFIX="${CMAKE_SHARED_MODULE_SUFFIX}")
    add_dependencies(authum_test_examples skeleton hello welcome chatfilter cvarguard settings
            stats scores scoreboard deathnotice announcer)
    add_test(NAME authum_examples COMMAND authum_test_examples)
endif ()

if(NOT WIN32)
    add_executable(authum_test_threads src/authum/host/threads.cpp)
    target_link_libraries(authum_test_threads PRIVATE authum_test_host authum_test_main)
    add_test(NAME authum_threads COMMAND authum_test_threads)
    set_tests_properties(authum_threads PROPERTIES TIMEOUT 10)
endif()

# Exercise the same HTTP implementation linked into the host image.
target_sources(authum_test_host PRIVATE "${CMAKE_SOURCE_DIR}/src/authum/host/clock.c")
if(NOT WIN32)
    file(GLOB AUTHUM_HTTP_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/authum/host/http/*.c")
    target_sources(authum_test_host PRIVATE ${AUTHUM_HTTP_TEST_SOURCES})
    add_executable(authum_test_http src/authum/host/http.cpp src/authum/host/clock.cpp)
    target_link_libraries(authum_test_http PRIVATE authum_test_host authum_test_main)
    add_test(NAME authum_http COMMAND authum_test_http)
    set_tests_properties(authum_http PROPERTIES TIMEOUT 10)
endif()

file(GLOB AUTHUM_LOG_WRITER_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/authum/host/logs/*.c")
target_sources(authum_test_host PRIVATE ${AUTHUM_LOG_WRITER_SOURCES})

# The placeholder registry the host binds to every module.
target_sources(authum_test_host PRIVATE "${CMAKE_SOURCE_DIR}/src/authum/host/placeholders.c")
add_executable(authum_test_placeholders src/authum/host/placeholders.cpp)
target_link_libraries(authum_test_placeholders PRIVATE authum_test_host authum_test_main)
add_test(NAME authum_placeholders COMMAND authum_test_placeholders)
set_tests_properties(authum_placeholders PROPERTIES TIMEOUT 15)

if(NOT WIN32)
    add_executable(authum_test_log_writer src/authum/host/logs/writer.cpp)
    target_link_libraries(authum_test_log_writer PRIVATE authum_test_host authum_test_main)
    add_test(NAME authum_log_writer COMMAND authum_test_log_writer)
    set_tests_properties(authum_log_writer PROPERTIES TIMEOUT 15)
endif()

add_executable(authum_test_memory src/authum/memory.cpp)
target_link_libraries(authum_test_memory PRIVATE authum_client authum_test_main)
add_test(NAME authum_memory COMMAND authum_test_memory)
