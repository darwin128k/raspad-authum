# Authum tests: plain C executables run by CTest. Host pieces (sandbox, events,
# commands) are compiled from src/authum together with lh, exactly as
# authum_mm uses them.

add_library(authum_test_check STATIC check.c)
target_include_directories(authum_test_check PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(authum_test_client client.c)
target_link_libraries(authum_test_client PRIVATE authum_client authum_test_check)
add_test(NAME authum_client COMMAND authum_test_client)

add_library(authum_test_host STATIC
        "${CMAKE_SOURCE_DIR}/src/authum/sandbox.c"
        "${CMAKE_SOURCE_DIR}/src/authum/events.c"
        "${CMAKE_SOURCE_DIR}/src/authum/commands.c"
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
