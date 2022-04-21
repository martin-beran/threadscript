# CMake prologue
cmake_minimum_required(VERSION 3.18.4)

project(
    ThreadScript
    LANGUAGES C CXX
)

include(CTest)

# Known build types
set(
    BUILD_TYPES
    Debug
    Release
    RelWithDebInfo
    MinSizeRel
)

# Known sanitizers
set(
    SANITIZERS 
    OFF
    address
    leak
    thread
    undefined
)

### BEGIN Build configuration #################################################

set(
    BUILD_TYPE "RelWithDebInfo" CACHE STRING
    "Select a build type (${BUILD_TYPES})"
)

set(
    CONFIG_FILE_HPP "threadscript/config_default.hpp" CACHE STRING
    "C++ source configuration file"
)

option(BUILD_SHARED_LIBS "Build shared (ON) or static (OFF) libraries" ON)

option(THREADSCRIPT_DEBUG "Enable use of threadscript::DEBUG class" OFF)

set(
    SANITIZER "OFF" CACHE STRING
    "Select a sanitizer (${SANITIZERS})"
)

### END Build configuration ###################################################

# Checking build configuration
set_property(
    CACHE BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES}
)
if (NOT BUILD_TYPE IN_LIST BUILD_TYPES)
    message(
        FATAL_ERROR "Unknown value of BUILD_TYPE: ${BUILD_TYPE}"
    )
endif()
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
endif()

if (BUILD_SHARED_LIBS)
    set(LIBS_TYPE "SHARED")
else()
    set(LIBS_TYPE "STATIC")
endif()

set_property(
    CACHE SANITIZER PROPERTY STRINGS ${SANITIZERS}
)
if (NOT SANITIZER IN_LIST SANITIZERS)
    message(FATAL_ERROR "Unknown value of SANITIZER: ${SANITIZER}")
endif()

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
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(
    -Wall -Wextra -pedantic -Werror
    -Wswitch-default -Wswitch-enum
)
include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src)
if (THREADSCRIPT_DEBUG)
    add_compile_definitions(THREADSCRIPT_DEBUG)
endif()
if (NOT SANITIZER STREQUAL "OFF")
    add_compile_options(
        -fsanitize=${SANITIZER}
    )
    add_link_options(
        -fsanitize=${SANITIZER}
    )
endif()

# We really want asserts in all builds
add_compile_options(-UNDEBUG)

# Used packages
find_package(Boost 1.74 REQUIRED OPTIONAL_COMPONENTS unit_test_framework)
find_package(Threads REQUIRED)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
if (NOT Boost_unit_test_framework_FOUND)
    set(BUILD_TESTING OFF)
endif()

# Optional Markdown utility for generating the top-level documentation stub
find_program(
    MARKDOWN
    NAMES markdown markdown_py
    DOC "Program for processing Markdown to HTML"
)

# Subdirectories with CMake configurations
add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(test)
endif()

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

add_custom_target(
    full
    COMMAND ${CMAKE_COMMAND} --build . -t clean
    COMMAND ${CMAKE_COMMAND} --build . -t all -j `nproc`
    COMMAND ${CMAKE_COMMAND} --build . -t doxygen
    COMMAND ${CMAKE_COMMAND} --build . -t clean-tidy
    COMMAND ${CMAKE_COMMAND} --build . -t clang-tidy -j `nproc`
    COMMAND ${CMAKE_COMMAND} --build . -t test -j `nproc`
)

# Report CMake configuration
message(STATUS "Version:     ${GIT_DESCRIBE}")
message(STATUS "Config file: ${CONFIG_FILE_HPP}")
message(STATUS "Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "Libraries:   ${LIBS_TYPE}")
message(STATUS "Doxygen:     ${DOXYGEN_FOUND} ${DOXYGEN_VERSION}")
message(STATUS "Markdown:    ${MARKDOWN}")
message(STATUS "Clang-tidy:  ${CLANG_TIDY}")
message(STATUS "Build tests: ${BUILD_TESTING}")
message(STATUS "DEBUG:       ${THREADSCRIPT_DEBUG}")
message(STATUS "Sanitizer:   ${SANITIZER}")
