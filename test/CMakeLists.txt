# Link Boost::Test library to test programs
link_libraries(
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    threadscript
    Threads::Threads
)

set(
    TEST_PROGRAMS
    allocated
    allocator_config
    channel
    code_node_resolve
    default_allocator
    dummy
    dummy_boost
    exception
    object
    parser
    parser_ascii
    predef
    shared_hash
    shared_vector
    symbol_table
    syntax
    syntax_canon
    ts
    vm_data
    virtual_machine
)

add_compile_definitions(TEST_SCRIPT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/scripts")
add_compile_definitions(TS_PROGRAM="${CMAKE_BINARY_DIR}/src/ts")

# Directory for files containing stdin, stdout, stderr of tests of program ts
set(TEST_IO_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_ts_io")
add_compile_definitions(TEST_IO_DIR="${TEST_IO_DIR}")
file(MAKE_DIRECTORY ${TEST_IO_DIR})

foreach (P ${TEST_PROGRAMS})
    add_executable(test_${P} test_${P}.cpp)
    add_test(NAME ${P} COMMAND test_${P})
endforeach()
