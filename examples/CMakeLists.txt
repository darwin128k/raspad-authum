# Example Authum modules. Each folder is one module built the way a real one
# is: a MODULE library that links authum_client and nothing else from Authum.
# Copy a folder next to your own project to start a module.

function(authum_example name)
    add_library(${name} MODULE ${ARGN})
    set_target_properties(${name} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${name}"
    )
    target_link_libraries(${name} PRIVATE authum_client)
    if (NOT MSVC)
        # Templates get copied: keep them free of warnings.
        target_compile_options(${name} PRIVATE -fvisibility=hidden -Wall -Wextra)
    endif ()
endfunction()

add_subdirectory(skeleton)
add_subdirectory(hello)
add_subdirectory(welcome)
add_subdirectory(chatfilter)
add_subdirectory(cvarguard)
add_subdirectory(settings)
add_subdirectory(stats)
add_subdirectory(scores)
add_subdirectory(scoreboard)
add_subdirectory(deathnotice)
add_subdirectory(announcer)
add_subdirectory(hookguard)
