/*! \file
 * \brief A dummy test program
 *
 * This program is used to prepare the test infrastructure.
 */

#include <cstdlib>
#include <string_view>

//! The main function of program \c test_dummy
/*! It returns \c EXIT_SUCCESS if run without command line arguments, or if
 * the first command line argument is \c "0". Otherwise, it returns \c
 * EXIT_FAILURE.
 * \param[in] argc the number of command line arguments
 * \param[in] argv the array of command line arguments
 * \return test result: \c EXIT_SUCCESS if passed,
 * \c EXIT_FAILURE if failed
 * \test Test description */
int main(int argc, char* argv[])
{
    using namespace std::string_view_literals;
    if (argc < 2 || argv[1] == "0"sv)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
