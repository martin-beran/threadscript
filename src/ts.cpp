/*! \file
 * \brief The command line ThreadScript interpreter
 *
 * This file contains complete source code of program \c ts that runs the
 * ThreadScript interpreter from the command line. It is a simple wrapper
 * around the ThreadScript C++ API, therefore it consists of a single file.
 *
 * See \ref Cmdline_ts for documentation of \c ts.
 */

#include "threadscript/threadscript.hpp"

#include <cstdlib>

//! Namespace containing declarations specific to program \c ts
/*! This namespace contains the complete implementation of program \c ts
 * except function main(). */
namespace pg_ts {

//! The function called in the main thread
constexpr std::string_view main_fun{"_main"};

//! The function called in each thread except the main
constexpr std::string_view thread_fun{"_thread"};

//! Exit status codes
enum class exit_status: int {
    success = EXIT_SUCCESS, //!< Successful termination
    failure = EXIT_FAILURE, //!< A generic failure
    unhandled_exception, //!< Terminated by an unhandled exception
    parse_error, //!< Terminated by an error during parsing the script
};

//! Processes command line arguments and holds the processed arguments.
/*! For simplicity, function \c getopt() is used to process command line
 * options.
 * \todo Explore possibilities of more sofisticated argument processing.
 * Candidates are:
 * \arg \c getopt_long()
 * \arg Boost.Program_options
 * \arg Some other existing command line processing library
 * \arg Creating a new command line processing library from scratch */
class args {
};

/*** args ********************************************************************/

} // namespace pg_ts

/*** entry point *************************************************************/

//! The main function of program \c ts
/*! \param[in] argc the number of command line arguments
 * \param[in] argv the array of command line arguments
 * \return the program exit status */
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    return EXIT_SUCCESS;
}
