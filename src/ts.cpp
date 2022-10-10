/*! \file
 * \brief The command line ThreadScript interpreter
 *
 * This file contains complete source code of program \c ts that runs the
 * ThreadScript interpreter from the command line. It is a simple wrapper
 * around the ThreadScript C++ API, therefore it consists of a single file.
 *
 * See \ref Cmdline_ts for documentation of \c ts.
 * \test in file test_ts.cpp
 */

#include "threadscript/threadscript.hpp"

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <unordered_set>

using namespace std::string_literals;
using namespace std::string_view_literals;

//! Namespace containing declarations specific to program \c ts
/*! This namespace contains the complete implementation of program \c ts
 * except function main(). */
namespace pg_ts {

//! The signed integer type compatible with type \c int in a script
using int_t = threadscript::config::value_int_type;

//! The unsigned integer type compatible with type \c unsigned in a script
using uint_t = threadscript::config::value_unsigned_type;

//! The variable containing command line arguments
constexpr std::string_view cmdline_var{"_cmdline"};

//! The function called in the main thread
constexpr std::string_view main_fun{"_main"};

//! The function called in each thread except the main
constexpr std::string_view thread_fun{"_thread"};

//! Exit status codes
/* \note The standard library defines \c EXIT_SUCCESS as 0 and \c EXIT_FAILURE
 * as 1.
 * \note The special exit status values are chosen so that they do not collide
 * with small values returned from a script. They are also compatible with
 * shell processing, which uses values from 128 for processes terminated by a
 * signal. */
enum class exit_status: int {
    success = EXIT_SUCCESS, //!< Successful termination
    failure = EXIT_FAILURE, //!< A generic failure
    unhandled_exception = 64, //!< Terminated by an unhandled exception
    args_error, //!< Invalid command line arguments
    parse_error, //!< Terminated by an error during parsing the script
};

//! The exception thrown if command line arguments are invalid
class args_error: public std::runtime_error {
    using runtime_error::runtime_error;
};

//! Processes command line arguments and holds the processed arguments.
/*! For simplicity, function \c getopt() is used to process command line
 * options.
 * \todo Explore possibilities of more sophisticated argument processing.
 * Candidates are:
 * \arg \c getopt_long()
 * \arg Boost.Program_options
 * \arg Some other existing command line processing library
 * \arg Creating a new command line processing library from scratch
 * \threadsafe{unsafe,unsafe} -- calls \c getopt(), which uses global variables
 */
class args {
public:
    //! Processes command line arguments.
    /*! Processed arguments are stored in member variables of this object.
     * Any argument values are copied from \a argv, therefore \a argc and \a
     * argv can be modified or destroyed freely after the constructor finishes.
     * \param[in] argc argument \a argc of main()
     * \param[in] argv argument \a argv of main()
     * \throw args_error if the arguments are invalid */
    args(int argc, char* argv[]);
    //! Gets the program name.
    /*! \return the program name from \c argv[0] */
    std::string program_name() const {
        return _program_name;
    }
    //! Gets the description of available command line arguments.
    /*! \return the message to be displayed to the user */
    std::string help_msg() const;
    //! Gets the script name.
    /*! \return the file name of the script to be executed */
    const std::string& script() const {
        return _script;
    }
    //! Gets the syntax variant.
    /*! \return the syntax used by the script */
    const std::string& syntax() const {
        return _syntax;
    }
    //! Gets the number of threads.
    /*! \return \c std::nullopt for a single-phase run; otherwise the number
     * of threads started in addition to the main thread */
    std::optional<int_t> threads() const {
        return _threads;
    }
    //! Gets the maximum allowed memory used by the script
    /*! \return the memory limit in bytes (always nonzero); \c std::nullopt for
     * no limit */
    std::optional<size_t> max_memory() const {
        return _max_memory;
    }
    //! Gets the maximum allowed stack depth
    /*! \return the limit of function call nesting (always nonzero); \c
     * std::nullopt for no limit */
    std::optional<size_t> max_stack() const {
        return _max_stack;
    }
    //! Gets arguments to be passed to the executed script
    /*! \return the vector of script arguments (after the script name) */
    const std::vector<std::string_view> script_args() const {
        return _script_args;
    }
    //! Request parsing a script, but not running it.
    /*! \return whether the script should be only parsed */
    bool parse_only() const {
        return _parse_only;
    }
    //! Request resolving names in the script after it is parsed.
    /*! \return whether to resolve names */
    bool resolve_parsed() const {
        return _resolve_parsed;
    }
    //! Request resolving names in the script after the first run phase.
    /*! \return whether to resolve names before running functions main_fun and
     * thread_fun */
    bool resolve_phase1() const {
        return _resolve_phase1;
    }
    //! Request reporting just the help message.
    /*! \return whether to report help (returned by help_msg()) */
    bool report_help() const {
        return _report_help;
    }
    //! Request reporting just the program version.
    /*! \return whether to report the version */
    bool report_version() const {
        return _report_version;
    }
    //! Request reporting some configuration information.
    /*! It requests reporting, e.g., size of integer types and supported syntax
     * variants.
     * \return whether to report the configuration */
    bool report_config() const {
        return _report_config;
    }
private:
    //! The stored program name
    std::string _program_name;
    //! The help message returned by help_msg()
    static std::string _help_msg;
    //! The script file name
    std::string _script = {};
    //! The syntax variant of _script
    std::string _syntax{threadscript::syntax_factory::syntax_canon};
    //! The number of additional threads
    std::optional<uint_t> _threads = {};
    //! The memory limit
    std::optional<size_t> _max_memory = {};
    //! The stack limit
    std::optional<size_t> _max_stack = {};
    //! Arguments passed to the script
    std::vector<std::string_view> _script_args;
    //! Flag for only parsing the script
    bool _parse_only = false;
    //! Resolve names in the script after parsing
    bool _resolve_parsed = false;
    //! Resolve names after the first run phase
    bool _resolve_phase1 = false;
    //! Flag for reporting help
    bool _report_help = false;
    //! Flag for reporting version
    bool _report_version = false;
    //! Flag for reporting configuration information
    bool _report_config = false;
};

/*** args ********************************************************************/

std::string args::_help_msg = R"([options] [--] script [args]

Arguments:

    --
        Terminates processing of options, useful if the script name starts with
        the character - and is not just - (the standard input).

    script
        The script file name. If it is -, the script is read from the standard
        input.

    args
        Any remaining arguments are passed to the script in global shared
        thread safe variable _cmdline of type vector containing elements of
        type

Options:

    -s SYNTAX
        Select the syntax variant of the script. If not used, the canonical
        syntax is expected.

    -t NUMBER
        Set the number of threads run in the second phase of the script
        execution. The main thread executes function _main, additional threads
        execute function _thread. If this options is zero, only the main thread
        is executed. If missing, only the first phase of execution is done.

    -M NUMBER
        NUMBER must be positive and controls how many bytes of memory the
        script can allocate. If the option is not used, memory is unlimited.

    -S NUMBER
        NUMBER must be positive and controls the maximum stack depth (the
        maximum nesting level of function calls). If the option is not used,
        the stack is unlimited.

    -n
        The script will be parsed and checked for syntax error, but it will not
        be executed.

    -R
        The predefined names of symbols (variables and functions) will be
        resolved in the script after it is parsed, before the first phase of
        execution starts.

    -r
        The names of symbols (variables and functions) will be resolved after
        the first phase of execution. It allows to resolve names of variables
        and functions defined during the first phase and use the resolved
        values in the second phase.

    -h
        Display this help message and exit.

    -v
        Display the program version.

    -C
        Display the program configuration. For example, it reports the program
        version, sizes of integer types, and supported syntax variants.

)";

args::args(int argc, char* argv[])
{
    assert(argc >= 1);
    _program_name = argv[0];
    std::unordered_set<char> used_opts;
    std::optional<char> only_opt;
    optind = 1;
    opterr = 0;
    for (int o;
         (o = getopt(argc, argv, "+s:t:M:S:nRrhvC")) != -1;
         used_opts.insert(o))
    {
        if (used_opts.contains(o))
            throw args_error("Repeated command line option -"s + char(o));
        size_t pos = 0;
        bool err = false;
        switch (o) {
        case 's':
            _syntax = optarg;
            break;
        case 't':
            try {
                _threads = std::stoull(optarg, &pos, 10);
                err = pos != strlen(optarg) ||
                    _threads > std::numeric_limits<
                                        decltype(_threads)::value_type>::max();
            } catch (...) {
                err = true;
            }
            break;
        case 'M':
            try {
                _max_memory = std::stoull(optarg, &pos, 10);
                err = pos != strlen(optarg) || _max_memory == 0 ||
                    _max_memory > std::numeric_limits<
                                    decltype(_max_memory)::value_type>::max();
            } catch (...) {
                err = true;
            }
            break;
        case 'S':
            try {
                _max_stack = std::stoull(optarg, &pos, 10);
                err = pos != strlen(optarg) || _max_stack == 0 ||
                    _max_stack > std::numeric_limits<
                                    decltype(_max_stack)::value_type>::max();
            } catch (...) {
                err = true;
            }
            break;
        case 'n':
            _parse_only = true;
            break;
        case 'R':
            _resolve_parsed = true;
            break;
        case 'r':
            _resolve_phase1 = true;
            break;
        case 'h':
            _report_help = true;
            if (!only_opt)
                only_opt = o;
            break;
        case 'v':
            _report_version = true;
            if (!only_opt)
                only_opt = o;
            break;
        case 'C':
            _report_config = true;
            if (!only_opt)
                only_opt = o;
            break;
        case '?':
        case ':':
        default:
            throw args_error("Invalid command line option -"s + char(optopt));
        }
        if (err)
            throw args_error("Invalid argument of command line option -"s +
                             char(o));
    }
    if (only_opt) {
        if (used_opts.size() > 1)
            throw args_error("Options -"s + char(*only_opt) +
                             " must be the only command line argument");
    } else {
        if (optind >= argc)
            throw args_error("Script name required");
        _script = argv[optind++];
        if (_script.empty())
            throw args_error("Empty script name");
        _script_args.reserve(argc - optind);
        for (; optind < argc; ++optind)
            _script_args.emplace_back(argv[optind]);
    }
}

std::string args::help_msg() const {
    return "\n"s + program_name() + " "s + _help_msg;
}

/*** actions *****************************************************************/

//! The actions requested by command line options
namespace actions {

//! Displays a help message on the standard output
/*! \param[in] a processed command line arguments
 * \return exit_status::success */
[[nodiscard]] exit_status help(const args& a)
{
    std::cout << a.help_msg();
    return exit_status::success;
}

//! Displays the program version (obtained by <tt>git describe</tt>)
/*! \param[in] a processed command line arguments
 * \return exit_status::success */
[[nodiscard]] exit_status version([[maybe_unused]] const args& a)
{
    std::cout << threadscript::version << '\n';
    return exit_status::success;
}

//! Displays the program configuration
/*! \param[in] a processed command line arguments
 * \return exit_status::success */
[[nodiscard]] exit_status config([[maybe_unused]] const args& a)
{
    using li = std::numeric_limits<threadscript::config::value_int_type>;
    using lu = std::numeric_limits<threadscript::config::value_unsigned_type>;
    std:: cout <<
        "Version:            " << threadscript::version << '\n' <<
        "Type int bits:      " << (li::digits + 1) << '\n' <<
        "Type int min:       " << li::min() << '\n' <<
        "Type int max:       +" << li::max() << '\n' <<
        "Type unsigned bits: " << lu::digits << '\n' <<
        "Type unsigned min:  " << lu::min() << '\n' <<
        "Type unsigned max:  " << lu::max() << '\n' <<
        "Syntax variants:   ";
    for (auto&& v: threadscript::syntax_factory::names())
        std::cout << ' ' << v;
    std::cout << '\n';
    return exit_status::success;
}

//! Parses and executes a script
/*! \param[in] a processed command line arguments
 * \return a result of script processing */
[[nodiscard]] exit_status script(const args& a)
{
    throw threadscript::exception::not_implemented(a.syntax() + ':' +
                                                   a.script());
}

} // namespace actions

} // namespace pg_ts

/*** entry point *************************************************************/

//! The main function of program \c ts
/*! \param[in] argc the number of command line arguments
 * \param[in] argv the array of command line arguments
 * \return the program exit status */
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    using namespace pg_ts;
    auto result = exit_status::success;
    try {
        const args main_args(argc, argv);
        if (main_args.report_help())
            result = actions::help(main_args);
        else if (main_args.report_version())
            result = actions::version(main_args);
        else if (main_args.report_config())
            result = actions::config(main_args);
        else
            result = actions::script(main_args);
    } catch (args_error& e) {
        std::cerr << argv[0] + ": "s + e.what() +
            "\nRun '"s + argv[0] + " -h' for help" << std::endl;
        result = exit_status::args_error;
    } catch (std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        result = exit_status::unhandled_exception;
    } catch (...) {
        std::cerr << "Unhandled unknown exception" << std::endl;
        result = exit_status::unhandled_exception;
    }
    return static_cast<int>(result);
}
