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
    vm_data
    virtual_machine
)

foreach (P ${TEST_PROGRAMS})
    add_executable(test_${P} test_${P}.cpp)
    add_test(NAME ${P} COMMAND test_${P})
endforeach()
