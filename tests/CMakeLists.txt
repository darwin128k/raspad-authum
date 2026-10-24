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
)
target_include_directories(authum_test_host PUBLIC "${CMAKE_SOURCE_DIR}/include")
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
