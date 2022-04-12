add_executable(test_dummy test_dummy.cpp)
add_test(NAME dummy COMMAND test_dummy)

# Link Boost::Test library to test program using Boost::Test
link_libraries(${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(test_dummy_boost test_dummy_boost.cpp)
add_test(NAME dummy_boost COMMAND test_dummy_boost)
