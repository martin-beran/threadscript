# CMake prologue
cmake_minimum_required(VERSION 3.18.4)

project(
    ThreadScript
    LANGUAGES C CXX
)

### BEGIN Build configuration #################################################

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(
    CONFIG_FILE_HPP "threadscript/config_default.hpp" CACHE STRING
    "C++ source configuration file"
)

option(BUILD_SHARED_LIBS "Build shared (ON) or static (OFF) libraries" ON)
if (BUILD_SHARED_LIBS)
    set(LIBS_TYPE "SHARED")
else()
    set(LIBS_TYPE "STATIC")
endif()

### END Build configuration ###################################################

# Version from Git
message(CHECK_START "Getting version by Git")
execute_process(
    COMMAND git describe
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (GIT_DESCRIBE)
    message(CHECK_PASS "OK")
else()
    message(CHECK_FAIL "FAIL")
    set(GIT_DESCRIBE "unknown")
endif()

# C++ options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(
    -Wall -Wextra -pedantic -Werror
    -Wswitch-default -Wswitch-enum
)
include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src)

# We really want asserts in all builds
add_compile_options(-UNDEBUG)

# Used packages
find_package(Boost 1.74 REQUIRED)
find_package(Threads REQUIRED)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

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
    add_custom_command(
        OUTPUT LICENSE.html
        COMMAND
            ${MARKDOWN} ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md > LICENSE.html
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md
    )
    list(APPEND DOC README.html LICENSE.html)
endif()

find_package(Doxygen)
if (DOXYGEN_FOUND)
    add_custom_target(
        doxygen
        COMMAND Doxygen::doxygen ${CMAKE_SOURCE_DIR}/Doxyfile > /dev/null
    )
endif()

add_custom_target(
    doc ALL DEPENDS ${DOC}
)

# Report CMake configuration
message(STATUS "Version:     ${GIT_DESCRIBE}")
message(STATUS "Config file: ${CONFIG_FILE_HPP}")
message(STATUS "Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "Libraries:   ${LIBS_TYPE}")
message(STATUS "Doxygen:     ${DOXYGEN_FOUND} ${DOXYGEN_VERSION}")
message(STATUS "Markdown:    ${MARKDOWN}")
message(STATUS "Clang-tidy:  ${CLANG_TIDY}")
