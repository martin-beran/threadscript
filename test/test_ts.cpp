/*! \file
 * \brief Tests of program \link ts.cpp ts\endlink
 *
 * The program is contained in file ts.cpp and namespace pg_ts.
 */

//! \cond
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE ts
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace test {

struct ts_result {
    std::vector<std::string> argv;
    std::string input;
    int status;
    std::function<bool(const std::string&)> test_output;
    std::function<bool(const std::string&)> test_error;
};

std::ostream& operator<<(std::ostream& os, const ts_result&)
{
    os << "runner_result";
    return os;
}

const std::filesystem::path script_dir{TEST_SCRIPT_DIR};
const std::filesystem::path ts_program{TS_PROGRAM};
const std::filesystem::path io_dir{TEST_IO_DIR};

std::string script_path(const std::string& script)
{
    return (script_dir / script).string();
}

void check_ts(std::string test, const ts_result& sample)
{
    test.erase(std::remove(test.begin(), test.end(), '/'), test.end());
    auto in_file = io_dir / (test + ".stdin");
    auto out_file = io_dir / (test + ".stdout");
    auto err_file = io_dir / (test + ".stderr");
    std::string cmd = ts_program;
    for (auto&& a:sample.argv)
        cmd.append(" ").append(a);
    cmd.append(" <").append(in_file);
    cmd.append(" >").append(out_file);
    cmd.append(" 2>").append(err_file);
    std::ofstream(in_file) << sample.input;
    int status = std::system(cmd.c_str());
    std::string out{
        (std::ostringstream{} << std::ifstream(out_file).rdbuf()).view()
    };
    std::string err{
        (std::ostringstream{} << std::ifstream(err_file).rdbuf()).view()
    };
    BOOST_CHECK_EQUAL(status % 256, 0);
    BOOST_CHECK_EQUAL(status / 256, sample.status);
    BOOST_CHECK(sample.test_output(out));
    BOOST_CHECK(sample.test_error(err));
}

} // namespace test
//! \endcond

/*! \file
 * \test \c no_args -- Program \link ts.cpp ts\endlink without arguments */
//! \cond
BOOST_DATA_TEST_CASE(no_args, (std::vector<test::ts_result>{
    {{}, "", 65,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s,
                std::regex(R"(Script name required\nRun '.*ts -h' for help)"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c bad_opts -- Program \link ts.cpp ts\endlink with invalid command
 * line options */
//! \cond
BOOST_DATA_TEST_CASE(bad_opts, (std::vector<test::ts_result>{
    {{"-1"}, "", 65, // unknown option
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s,
                std::regex(R"(Invalid command line option -1\n)"
                           R"(Run '.*ts -h' for help)"));
        }
    },
    {{"-t"}, "", 65, // missing option value
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s,
                std::regex(R"(Invalid command line option -t\n)"
                           R"(Run '.*ts -h' for help)"));
        }
    },
    {{"-t", "X"}, "", 65, // bad option value
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s,
                std::regex(R"(Invalid argument of command line option -t\n)"
                           R"(Run '.*ts -h' for help)"));
        }
    },
    {{"-h", "-v"}, "", 65, // invalid combination of options
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s,
            std::regex(R"(Option -h must be the only command line argument\n)"
                       R"(Run '.*ts -h' for help)"));
        }
    },
    {{"-v", "-n"}, "", 65, // invalid combination of options
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s,
            std::regex(R"(Option -v must be the only command line argument\n)"
                       R"(Run '.*ts -h' for help)"));
        }
    },
    {{"-t", "3", "-C"}, "", 65, // invalid combination of options
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s,
            std::regex(R"(Option -C must be the only command line argument\n)"
                       R"(Run '.*ts -h' for help)"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c opts_end -- Program \link ts.cpp ts\endlink uses \c "--" as the end
 * of options */
//! \cond
BOOST_DATA_TEST_CASE(opts_end, (std::vector<test::ts_result>{
    {{"--", "-n", "hello.ts"}, "", 66,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s, std::regex(R"(Cannot parse -n: )"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c script_file -- Program \link ts.cpp ts\endlink with a script file */
//! \cond
BOOST_DATA_TEST_CASE(script_file, (std::vector<test::ts_result>{
    {{test::script_path("hello.ts")}, "", 0,
        [](auto&& s) { return s == "Hello World!\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{test::script_path("syntax_error.ts")}, "", 66,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                    R"(Cannot parse /.*/syntax_error.ts: 1:8: Expected '\(')"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c no_script_file -- Program \link ts.cpp ts\endlink with a script
 * file that does not exist */
//! \cond
BOOST_DATA_TEST_CASE(no_script_file, (std::vector<test::ts_result>{
    {{test::script_path("script-file-does-not-exist.ts")}, "", 66,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                        R"(Cannot parse /.*/script-file-does-not-exist.ts: )"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c script_stdin -- Program \link ts.cpp ts\endlink with a script on
 * standard input */
//! \cond
BOOST_DATA_TEST_CASE(script_stdin, (std::vector<test::ts_result>{
    {{"-"}, R"(print("Hello World!", "\n"))", 0,
        [](auto&& s) { return s == "Hello World!\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "syntax error", 66,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                                    R"(Cannot parse -: 1:8: Expected '\(')"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c help -- Program \link ts.cpp ts\endlink help output */
//! \cond
BOOST_DATA_TEST_CASE(help, (std::vector<test::ts_result>{
    {{"-h"}, "", 0,
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                R"(Usage: .*ts .*script(.|\n)*Arguments:(.|\n)*Options:)"));
        },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c version -- Program \link ts.cpp ts\endlink version output */
//! \cond
BOOST_DATA_TEST_CASE(version, (std::vector<test::ts_result>{
    {{"-v"}, "", 0,
        [](auto&& s) {
            return s == std::string(threadscript::version) + "\n"s;
        },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c config -- Program \link ts.cpp ts\endlink configuration output */
//! \cond
BOOST_DATA_TEST_CASE(config, (std::vector<test::ts_result>{
    {{"-C"}, "", 0,
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                R"(Version: +\S+\n)"
                R"(Type int bits: +\d+\n)"
                R"(Type int min: +-\d+\n)"
                R"(Type int max: +\+\d+\n)"
                R"(Type unsigned bits: +\d+\n)"
                R"(Type unsigned min: +\d+\n)"
                R"(Type unsigned max: +\d+\n)"
                R"(Syntax variants: +\S.*\n)"));
        },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c not_run -- Program \link ts.cpp ts\endlink with option \c -n parses
 * the script, but does not execute it */
//! \cond
BOOST_DATA_TEST_CASE(not_run, (std::vector<test::ts_result>{
    {{"-n", "-"}, R"(print("Hello World!", "\n"))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-n", test::script_path("hello.ts")}, "", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-n", "-"}, "syntax error", 66,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                                    R"(Cannot parse -: 1:8: Expected '\(')"));
        }
    },
    {{"-n", test::script_path("syntax_error.ts")}, "", 66,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                R"(Cannot parse /.*/syntax_error\.ts: 1:8: Expected '\(')"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c syntax -- Program \link ts.cpp ts\endlink selects a syntax variant
 * by option \c -s */
//! \cond
BOOST_DATA_TEST_CASE(syntax, (std::vector<test::ts_result>{
    {{"-s", "canon", "-"}, R"(print("Hello World!", "\n"))", 0,
        [](auto&& s) { return s == "Hello World!\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-s", "canon", test::script_path("hello.ts")}, "", 0,
        [](auto&& s) { return s == "Hello World!\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-s", "Undefined", "-"}, R"(print("Hello World!", "\n"))", 66,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                R"(Cannot parse -: Parse error: Unknown syntax "Undefined")"));
        }
    },
    {{"-s", "Undefined", test::script_path("hello.ts")}, "", 66,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                R"(Cannot parse /.*/hello\.ts: )"
                R"(Parse error: Unknown syntax "Undefined")"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c script_args -- Program \link ts.cpp ts\endlink passes remaining
 * command line arguments after script name to the script */
//! \cond
BOOST_DATA_TEST_CASE(script_args, (std::vector<test::ts_result>{
    {{test::script_path("print_args.ts")}, "", 0,
        [](auto&& s) { return s == "argc=0\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{test::script_path("print_args.ts"), "ARG1"}, "", 0,
        [](auto&& s) { return s == "argc=1\nargv[0]=\"ARG1\"\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{test::script_path("print_args.ts"), "''", "a", "bc", "def"}, "", 0,
        [](auto&& s) {
            return s == "argc=4\nargv[0]=\"\"\nargv[1]=\"a\"\n"
                "argv[2]=\"bc\"\nargv[3]=\"def\"\n";
        },
        [](auto&& s) { return s.empty(); }
    },
    {{test::script_path("print_args.ts"), "-o", "arg"}, "", 0,
        [](auto&& s) {
            return s == "argc=2\nargv[0]=\"-o\"\nargv[1]=\"arg\"\n";
        },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c status_phase1 -- Program \link ts.cpp ts\endlink uses the result of
 * script evaluation as the process exit status in a single-phase run. */
//! \cond
BOOST_DATA_TEST_CASE(status_phase1, (std::vector<test::ts_result>{
    {{"-"}, "null", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "false", 1,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "true", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "0", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "1", 1,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "2", 2,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "+0", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "+10", 10,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "\"str\"", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "vector()", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-"}, "hash()", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c status_phase1_exc -- Program \link ts.cpp ts\endlink terminates
 * with status pg_ts::exit_status::run_exception if a script throws an
 * exception during the first phase of a run. */
//! \cond
BOOST_DATA_TEST_CASE(status_phase1_exc, (std::vector<test::ts_result>{
    {{"-q", "-"}, R"(throw("EXCEPTION"))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Script terminated by exception: -:1:1:(): "
                "Script exception: EXCEPTION\n";
        }
    },
    {{"-"}, R"(throw("EXCEPTION"))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Script terminated by exception: -:1:1:(): "
                "Script exception: EXCEPTION\n"
                "    0. -:1:1:()\n\n";
        }
    },
    {{"-q", "-"}, R"(seq(
            fun("exc", throw("EXCEPTION")),
            fun("g", exc()),
            fun("f", g()),
            f()
        ))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Script terminated by exception: -:2:24:exc(): "
                "Script exception: EXCEPTION\n";
        }
    },
    {{"-"}, R"(seq(
            fun("exc", throw("EXCEPTION")),
            fun("g", exc()),
            fun("f", g()),
            f()
        ))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Script terminated by exception: -:2:24:exc(): "
                "Script exception: EXCEPTION\n"
                "    0. -:2:24:exc()\n"
                "    1. -:3:22:g()\n"
                "    2. -:4:22:f()\n"
                "    3. -:5:13:()\n\n";
        }
    },
    {{"-"}, R"(seq(
            fun("_main", throw("MAIN")),
            fun("_thread", throw("THREAD"))
        ))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c status_phase2 -- Program \link ts.cpp ts\endlink uses the return
 * value of function \c _main as the process exit status in a two-phase run. */
//! \cond
BOOST_DATA_TEST_CASE(status_phase2, (std::vector<test::ts_result>{
    {{"-t", "0", "-"}, R"(seq(fun("_main", null), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", null), fun("_thread", 10), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", false), 20))", 1,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", false), fun("_thread", 10), 20))", 1,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", true), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", true), fun("_thread", 10), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", 0), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", 0), fun("_thread", 10), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", 1), 20))", 1,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", 1), fun("_thread", 10), 20))", 1,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", 5), 20))", 5,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", 5), fun("_thread", 10), 20))", 5,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", +0), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", +0), fun("_thread", 10), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", +1), 20))", 1,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", +1), fun("_thread", 10), 20))", 1,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", +5), 20))", 5,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", +5), fun("_thread", 10), 20))", 5,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", "str"), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", "str"), fun("_thread", 10), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", vector()), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", vector()), fun("_thread", 10), 20))",
        0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", hash()), 20))", 0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(fun("_main", hash()), fun("_thread", 10), 20))",
        0,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c status_phase2_exc_main -- Program \link ts.cpp ts\endlink
 * terminates with status pg_ts::exit_status::run_exception if function \c
 * _main throws an exception during the second phase of a run. */
//! \cond
BOOST_DATA_TEST_CASE(status_phase2_exc, (std::vector<test::ts_result>{
    {{"-q", "-t", "0", "-"}, R"(seq(fun("_main", throw("EXC")), 20))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Main thread terminated by exception: -:1:18:_main(): "
                "Script exception: EXC\n";
        }
    },
    {{"-t", "0", "-"}, R"(seq(fun("_main", throw("EXC")), 20))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Main thread terminated by exception: -:1:18:_main(): "
                "Script exception: EXC\n"
                "    0. -:1:18:_main()\n\n";
        }
    },
    {{"-q", "-t", "0", "-"},
        R"(seq(fun("_main", throw("EXC")), fun("_thread", 10), 20))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Main thread terminated by exception: -:1:18:_main(): "
                "Script exception: EXC\n";
        }
    },
    {{"-t", "0", "-"},
        R"(seq(fun("_main", throw("EXC")), fun("_thread", 10), 20))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Main thread terminated by exception: -:1:18:_main(): "
                "Script exception: EXC\n"
                "    0. -:1:18:_main()\n\n";
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c status_phase2_exc_thread -- Program \link ts.cpp ts\endlink
 * terminates with status pg_ts::exit_status::thread_exception if function \c
 * _main does not throw and function \c _thread throws an exception in at least
 * one thread during the second phase of a run. */
//! \cond
BOOST_DATA_TEST_CASE(status_phase2_exc_thread, (std::vector<test::ts_result>{
    {{"-q", "-t", "1", "-"}, R"(seq(
            fun("_main", null),
            fun("_thread", throw("Thread"))
        ))", 69,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == R"(Thread 0 terminated by exception: )"
                R"(-:3:28:_thread(): Script exception: Thread)" "\n";
        }
    },
    {{"-t", "1", "-"}, R"(seq(
            fun("_main", null),
            fun("_thread", throw("Thread"))
        ))", 69,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Thread 0 terminated by exception: "
                "-:3:28:_thread(): Script exception: Thread\n"
                "    0. -:3:28:_thread()\n\n";
        }
    },
    {{"-q", "-t", "5", "-"}, R"(seq(
            fun("_main", null),
            fun("_thread", if(eq(at(_args(), 0), 2), throw("Thread")))
        ))", 69,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == R"(Thread 2 terminated by exception: )"
                R"(-:3:54:_thread(): Script exception: Thread)" "\n";
        }
    },
    {{"-t", "5", "-"}, R"(seq(
            fun("_main", null),
            fun("_thread", if(eq(at(_args(), 0), 2), throw("Thread")))
        ))", 69,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Thread 2 terminated by exception: "
                "-:3:54:_thread(): Script exception: Thread\n"
                "    0. -:3:54:_thread()\n\n";
        }
    },
    {{"-t", "1", "-"}, R"(seq(
            fun("_main", throw("EXC")),
            fun("_thread", throw("Thread")),
            20
        ))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s.find("Main thread terminated by exception: -:2:26:"
                    "_main(): Script exception: EXC\n") != std::string::npos &&
                s.find("Thread 0 terminated by exception: -:3:28:_thread(): "
                    "Script exception: Thread\n") != std::string::npos;
        }
    },
    {{"-q", "-t", "1", "-"}, R"(seq(
            fun("_main", 67), # pg_ts::exit_status::run_exception
            fun("_thread", throw("Thread"))
        ))", 69,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == R"(Thread 0 terminated by exception: )"
                R"(-:3:28:_thread(): Script exception: Thread)" "\n";
        }
    },
    {{"-t", "1", "-"}, R"(seq(
            fun("_main", 67), # pg_ts::exit_status::run_exception
            fun("_thread", throw("Thread"))
        ))", 69,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Thread 0 terminated by exception: "
                "-:3:28:_thread(): Script exception: Thread\n"
                "    0. -:3:28:_thread()\n\n";
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c single_phase -- Program \link ts.cpp ts\endlink executes the script
 * in a single-phase run. */
//! \cond
BOOST_DATA_TEST_CASE(single_phase, (std::vector<test::ts_result>{
    {{"-"}, R"(seq(
            fun("_main", print("main\n")),
            fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 0,
        [](auto&& s) { return s == "script\n"; },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c no_threads -- Program \link ts.cpp ts\endlink executes function
 * _main in a two-phase run without additional threads. */
//! \cond
BOOST_DATA_TEST_CASE(no_threads, (std::vector<test::ts_result>{
    {{"-t", "0", "-"}, R"(seq(
            fun("_main", print("main\n")),
            fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 0,
        [](auto&& s) { return s == "script\nmain\n"; },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c threads -- Program \link ts.cpp ts\endlink executes functions
 * _main and _threads in a two-phase run with additional threads. */
//! \cond
BOOST_DATA_TEST_CASE(threads, (std::vector<test::ts_result>{
    {{"-t", "1", "-"}, R"(seq(
            fun("_main", print("main\n")),
            fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 0,
        [](auto&& s) {
            return s.find("script\n") != std::string::npos &&
                s.find("main\n") != std::string::npos &&
                s.find("thread 0\n") != std::string::npos;
        },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "10", "-"}, R"(seq(
            fun("_main", print("main\n")),
            fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 0,
        [](auto&& s) {
            bool ok = s.find("script\n") != std::string::npos &&
                s.find("main\n") != std::string::npos;
            for (int i = 0; i < 10; ++i)
                ok = ok && s.find("thread " + std::to_string(i) +
                                  "\n") != std::string::npos;
            return ok;
        },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c no_fun_main -- Program \link ts.cpp ts\endlink fails in a two-phase
 * run if function \c _main is missing. */
//! \cond
BOOST_DATA_TEST_CASE(no_fun_main, (std::vector<test::ts_result>{
    {{"-"}, R"(seq(
            #fun("_main", print("main\n")),
            fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 0,
        [](auto&& s) { return s == "script\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(
            #fun("_main", print("main\n")),
            fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 68,
        [](auto&& s) { return s == "script\n"; },
        [](auto&& s) { return s == "Function _main not defined\n"; }
    },
    {{"-t", "1", "-"}, R"(seq(
            #fun("_main", print("main\n")),
            fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 68,
        [](auto&& s) { return s == "script\n"; },
        [](auto&& s) { return s == "Function _main not defined\n"; }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c no_fun_threads -- Program \link ts.cpp ts\endlink fails in a
 * two-phase run with additional threads if function \c _threads is missing. */
//! \cond
BOOST_DATA_TEST_CASE(no_fun_threads, (std::vector<test::ts_result>{
    {{"-"}, R"(seq(
            fun("_main", print("main\n")),
            #fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 0,
        [](auto&& s) { return s == "script\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-"}, R"(seq(
            fun("_main", print("main\n")),
            #fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 0,
        [](auto&& s) { return s == "script\nmain\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "1", "-"}, R"(seq(
            fun("_main", print("main\n")),
            #fun("_thread", print("thread ", at(_args(), 0), "\n")),
            print("script\n")
        ))", 68,
        [](auto&& s) { return s == "script\n"; },
        [](auto&& s) { return s == "Function _thread not defined\n"; }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c resolve1 -- Program \link ts.cpp ts\endlink resolves symbol names
 * before the first phase of execution. */
//! \cond
BOOST_DATA_TEST_CASE(resolve1, (std::vector<test::ts_result>{
    {{"-"}, R"(seq(
            fun("bool", "redefined bool"),
            print(bool(1))
        ))", 0,
        [](auto&& s) { return s == "redefined bool"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-R", "-"}, R"(seq(
            fun("bool", "redefined bool"),
            print(bool(1))
        ))", 0,
        [](auto&& s) { return s == "true"; },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c resolve2 -- Program \link ts.cpp ts\endlink resolves symbol names
 * before the second phase of execution. */
//! \cond
BOOST_DATA_TEST_CASE(resolve2, (std::vector<test::ts_result>{
    {{"-t", "0", "-"}, R"(seq(
            fun("_main", seq(
                fun("bool", "bool2"),
                fun("f", "main"),
                print(bool(1), " ", f(), "\n")
            )),
            fun("bool", "bool1"),
            fun("f", "script"),
            print(bool(1), " ", f(), "\n")
        ))", 0,
        [](auto&& s) { return s == "bool1 script\nbool2 main\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-R", "-"}, R"(seq(
            fun("_main", seq(
                fun("bool", "bool2"),
                fun("f", "main"),
                print(bool(1), " ", f(), "\n")
            )),
            fun("bool", "bool1"),
            fun("f", "script"),
            print(bool(1), " ", f(), "\n")
        ))", 0,
        [](auto&& s) { return s == "true script\ntrue main\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-r", "-"}, R"(seq(
            fun("_main", seq(
                fun("bool", "bool2"),
                fun("f", "main"),
                print(bool(1), " ", f(), "\n")
            )),
            fun("bool", "bool1"),
            fun("f", "script"),
            print(bool(1), " ", f(), "\n")
        ))", 0,
        [](auto&& s) { return s == "bool1 script\nbool1 script\n"; },
        [](auto&& s) { return s.empty(); }
    },
    {{"-t", "0", "-R", "-r", "-"}, R"(seq(
            fun("_main", seq(
                fun("bool", "bool2"),
                fun("f", "main"),
                print(bool(1), " ", f(), "\n")
            )),
            fun("bool", "bool1"),
            fun("f", "script"),
            print(bool(1), " ", f(), "\n")
        ))", 0,
        [](auto&& s) { return s == "true script\nbool1 script\n"; },
        [](auto&& s) { return s.empty(); }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c memory_limit -- Program \link ts.cpp ts\endlink fails if a script
 * exceeds a memory limit. */
//! \cond
BOOST_DATA_TEST_CASE(memory_limit, (std::vector<test::ts_result>{
    {{"-q", "-M", "100000", "-"}, R"(seq(
            var("v", vector()),
            var("i", clone(0)),
            while(true, seq(
                at(v(), i(), "vector element"),
                add(i(), i(), 1)
            ))
        ))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return std::regex_match(s, std::regex(
                R"(Script terminated by exception: -:\d+:\d+:.*\(\): )"s +
                std::bad_alloc().what() + R"(\n)"));
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond

/*! \file
 * \test \c stack_limit -- Program \link ts.cpp ts\endlink fails if a script
 * exceeds a stack depth limit. */
//! \cond
BOOST_DATA_TEST_CASE(stack_limit, (std::vector<test::ts_result>{
    {{"-S", "5", "-"}, R"(seq(
            fun("f", f()),
            f()
        ))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Script terminated by exception: -:2:22:f(): "
                "Runtime error: Recursion too deep\n"
                "    0. -:2:22:f()\n"
                "    1. -:2:22:f()\n"
                "    2. -:2:22:f()\n"
                "    3. -:2:22:f()\n"
                "    4. -:3:13:()\n\n";
        }
    },
    {{"-t", "0", "-S", "5", "-"}, R"(seq(
            fun("f", f()),
            fun("_main", f())
        ))", 67,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Main thread terminated by exception: -:2:22:f(): "
                "Runtime error: Recursion too deep\n"
                "    0. -:2:22:f()\n"
                "    1. -:2:22:f()\n"
                "    2. -:2:22:f()\n"
                "    3. -:2:22:f()\n"
                "    4. -:3:26:_main()\n\n";
        }
    },
    {{"-t", "1", "-S", "5", "-"}, R"(seq(
            fun("f", f()),
            fun("_main", null),
            fun("_thread", f())
        ))", 69,
        [](auto&& s) { return s.empty(); },
        [](auto&& s) {
            return s == "Thread 0 terminated by exception: -:2:22:f(): "
                "Runtime error: Recursion too deep\n"
                "    0. -:2:22:f()\n"
                "    1. -:2:22:f()\n"
                "    2. -:2:22:f()\n"
                "    3. -:2:22:f()\n"
                "    4. -:4:28:_thread()\n\n";
        }
    },
}))
{
    check_ts(boost::unit_test::framework::current_test_case().full_name(),
             sample);
}
//! \endcond
