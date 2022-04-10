# Generated build configuration file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/threadscript/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/threadscript/config.hpp
)

# ThreadScript library
add_library(
    libthreadscript
    threadscript.cpp
)

# Command line interpreter ts
add_executable(ts ts.cpp)
target_link_libraries(ts PRIVATE libthreadscript Threads::Threads)
