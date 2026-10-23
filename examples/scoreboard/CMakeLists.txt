# Reads the owner's public header (types only); never links the scores module.
authum_example(scoreboard scoreboard.c)
target_include_directories(scoreboard PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../scores")
