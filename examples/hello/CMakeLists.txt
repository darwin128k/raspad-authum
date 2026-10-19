# Client module: Authum headers only — does NOT link lh.
add_library(hello MODULE hello.c)
set_target_properties(hello PROPERTIES
        PREFIX ""
        OUTPUT_NAME "hello"
)

target_include_directories(hello PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
)

if (WIN32)
    target_compile_definitions(hello PRIVATE WIN32 _WINDOWS)
else ()
    target_compile_definitions(hello PRIVATE linux LINUX)
    if (NOT MSVC)
        target_compile_options(hello PRIVATE -fvisibility=hidden)
    endif ()
endif ()
