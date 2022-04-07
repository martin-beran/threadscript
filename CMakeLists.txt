# CMake prologue
cmake_minimum_required(VERSION 3.18.4)

project(ThreadScript)

# We require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Optional Markdown utility for generating the top-level documentation stub
find_program(
    MARKDOWN
    NAMES markdown markdown_py
    DOC "Program for processing Markdown to HTML"
)

# Subdirectories with CMake configurations
add_subdirectory(src)
add_subdirectory(test)

# Generate documentation, optional (Markdown and Doxygen)
set(DOC "")

if (MARKDOWN)
    add_custom_command(
        OUTPUT README.html
        COMMAND ${MARKDOWN} ${CMAKE_CURRENT_SOURCE_DIR}/README.md > README.html
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    )
    list(APPEND DOC README.html)
endif()

find_package(Doxygen)
if (DOXYGEN_FOUND)
    add_custom_target(
        doxygen
        COMMAND Doxygen::doxygen ${CMAKE_SOURCE_DIR}/Doxyfile
    )
endif()

add_custom_target(
    doc ALL DEPENDS ${DOC}
)

# Report CMake configuration
message(STATUS "Doxygen:  ${DOXYGEN_FOUND} ${DOXYGEN_VERSION}")
message(STATUS "Markdown: ${MARKDOWN}")
