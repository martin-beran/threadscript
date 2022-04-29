# Link Boost::Test library to test programs
link_libraries(
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    libthreadscript
    Threads::Threads
)

set(
    TEST_PROGRAMS
    allocated
    allocator_config
    default_allocator
    dummy
    dummy_boost
    exception
    vm_data
    virtual_machine
)

foreach (P ${TEST_PROGRAMS})
    add_executable(test_${P} test_${P}.cpp)
    add_test(NAME ${P} COMMAND test_${P})
endforeach()
