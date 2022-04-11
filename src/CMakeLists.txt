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

# Check sources by clang-tidy
find_program(CLANG_TIDY NAMES clang-tidy)
if (CLANG_TIDY)
    file(
        GLOB_RECURSE CLANG_TIDY_SRC
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_SOURCE_DIR}
        *.cpp
    )
    list(TRANSFORM CLANG_TIDY_SRC APPEND .tidy OUTPUT_VARIABLE CLANG_TIDY_OUT)
    list(TRANSFORM CLANG_TIDY_SRC PREPEND ${CMAKE_SOURCE_DIR}/)
    list(TRANSFORM CLANG_TIDY_OUT PREPEND ${CMAKE_BINARY_DIR}/)
    message(STATUS "CLANG_TIDY_SRC=${CLANG_TIDY_SRC}")
    message(STATUS "CLANG_TIDY_OUT=${CLANG_TIDY_OUT}")

    add_custom_target(
        clang-tidy
        COMMAND test `cat ${CLANG_TIDY_OUT} | wc -c` = 0
        DEPENDS ${CLANG_TIDY_OUT}
    )
    foreach (S O IN ZIP_LISTS CLANG_TIDY_SRC CLANG_TIDY_OUT)
        add_custom_command(
            OUTPUT ${O}
            COMMAND ${CLANG_TIDY} ${S} > ${O}
            DEPENDS ${S}
        )
    endforeach()

    add_custom_target(clean-tidy COMMAND rm -f ${CLANG_TIDY_OUT})
endif()
