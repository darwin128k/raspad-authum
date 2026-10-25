# Authum tests: plain C executables run by CTest. Host pieces (sandbox, events,
# commands, logs) are compiled from src/authum/host together with lh, exactly as
# authum uses them.

add_library(authum_test_check STATIC check.c)
target_include_directories(authum_test_check PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(authum_test_client client.c)
target_link_libraries(authum_test_client PRIVATE authum_client authum_test_check)
add_test(NAME authum_client COMMAND authum_test_client)

add_library(authum_test_host STATIC
        "${CMAKE_SOURCE_DIR}/src/authum/host/sandbox.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/events.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/commands.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/logs.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/tasks.c"
        "${CMAKE_SOURCE_DIR}/src/authum/host/threads.c"
)
target_include_directories(authum_test_host PUBLIC "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(authum_test_host PUBLIC lh authum_client)

add_executable(authum_test_sandbox sandbox.c)
target_link_libraries(authum_test_sandbox PRIVATE authum_test_host authum_test_check)
add_test(NAME authum_sandbox COMMAND authum_test_sandbox)

add_executable(authum_test_events events.c)
target_link_libraries(authum_test_events PRIVATE authum_test_host authum_test_check)
add_test(NAME authum_events COMMAND authum_test_events)

add_executable(authum_test_commands commands.c)
target_link_libraries(authum_test_commands PRIVATE authum_test_host authum_test_check)
add_test(NAME authum_commands COMMAND authum_test_commands)

add_executable(authum_test_timers timers.c)
target_link_libraries(authum_test_timers PRIVATE authum_test_host authum_test_check)
add_test(NAME authum_timers COMMAND authum_test_timers)

add_executable(authum_test_logs logs.c)
target_link_libraries(authum_test_logs PRIVATE authum_test_host authum_test_check)
add_test(NAME authum_logs COMMAND authum_test_logs)

add_executable(authum_test_tasks tasks.c)
target_link_libraries(authum_test_tasks PRIVATE authum_test_host authum_test_check)
add_test(NAME authum_tasks COMMAND authum_test_tasks)
set_tests_properties(authum_tasks PROPERTIES TIMEOUT 20)

add_executable(authum_test_common common.c)
target_link_libraries(authum_test_common PRIVATE authum_test_host authum_test_check)
add_test(NAME authum_common COMMAND authum_test_common)
set_tests_properties(authum_common PROPERTIES TIMEOUT 20)

# Smoke test of examples/: loads every built example module.
if (AUTHUM_BUILD_EXAMPLES)
    add_executable(authum_test_examples examples.c)
    target_link_libraries(authum_test_examples PRIVATE authum_test_host authum_test_check
            ${CMAKE_DL_LIBS})
    target_compile_definitions(authum_test_examples PRIVATE
            AUTHUM_TEST_EXAMPLES_DIR="${CMAKE_BINARY_DIR}/examples"
            AUTHUM_TEST_MODULE_SUFFIX="${CMAKE_SHARED_MODULE_SUFFIX}")
    add_dependencies(authum_test_examples skeleton hello welcome chatfilter cvarguard settings
            stats scores scoreboard deathnotice announcer)
    add_test(NAME authum_examples COMMAND authum_test_examples)
endif ()

if(NOT WIN32)
    add_executable(authum_test_threads threads.c)
    target_link_libraries(authum_test_threads PRIVATE authum_test_host authum_test_check)
    add_test(NAME authum_threads COMMAND authum_test_threads)
    set_tests_properties(authum_threads PROPERTIES TIMEOUT 10)
endif()

# Exercise the same HTTP implementation linked into the host image.
target_sources(authum_test_host PRIVATE "${CMAKE_SOURCE_DIR}/src/authum/host/clock.c")
if(NOT WIN32)
    file(GLOB AUTHUM_HTTP_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/authum/host/http/*.c")
    target_sources(authum_test_host PRIVATE ${AUTHUM_HTTP_TEST_SOURCES} "${CMAKE_SOURCE_DIR}/lib/picohttpparser/picohttpparser.c")
    target_include_directories(authum_test_host PRIVATE "${CMAKE_SOURCE_DIR}/lib/picohttpparser")
    add_executable(authum_test_http http.c)
    target_link_libraries(authum_test_http PRIVATE authum_test_host authum_test_check)
    add_test(NAME authum_http COMMAND authum_test_http)
    set_tests_properties(authum_http PROPERTIES TIMEOUT 10)
endif()

file(GLOB AUTHUM_LOG_WRITER_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/authum/host/logs/*.c")
target_sources(authum_test_host PRIVATE ${AUTHUM_LOG_WRITER_SOURCES})

if(NOT WIN32)
    add_executable(authum_test_log_writer log_writer.c)
    target_compile_options(authum_test_log_writer PRIVATE -UNDEBUG)
    target_link_libraries(authum_test_log_writer PRIVATE authum_test_host)
    add_test(NAME authum_log_writer COMMAND authum_test_log_writer)
    set_tests_properties(authum_log_writer PROPERTIES TIMEOUT 15)
endif()
