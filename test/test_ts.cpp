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
        [](auto&& s) {
            return s.empty();
        },
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
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return std::regex_search(s,
                std::regex(R"(Invalid command line option -1\n)"
                           R"(Run '.*ts -h' for help)")); 
        }
    },
    {{"-t"}, "", 65, // missing option value
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return std::regex_search(s,
                std::regex(R"(Invalid command line option -t\n)"
                           R"(Run '.*ts -h' for help)")); 
        }
    },
    {{"-t", "X"}, "", 65, // bad option value
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return std::regex_search(s,
                std::regex(R"(Invalid argument of command line option -t\n)"
                           R"(Run '.*ts -h' for help)")); 
        }
    },
    {{"-h", "-v"}, "", 65, // invalid combination of options
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return std::regex_search(s,
            std::regex(R"(Option -h must be the only command line argument\n)"
                       R"(Run '.*ts -h' for help)")); 
        }
    },
    {{"-v", "-n"}, "", 65, // invalid combination of options
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return std::regex_search(s,
            std::regex(R"(Option -v must be the only command line argument\n)"
                       R"(Run '.*ts -h' for help)")); 
        }
    },
    {{"-t", "3", "-C"}, "", 65, // invalid combination of options
        [](auto&& s) {
            return s.empty();
        },
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
        [](auto&& s) {
            return s.empty();
        },
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
        [](auto&& s) {
            return s == "Hello World!\n";
        },
        [](auto&& s) {
            return s.empty();
        }
    },
    {{test::script_path("syntax_error.ts")}, "", 66,
        [](auto&& s) {
            return s.empty();
        },
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
        [](auto&& s) {
            return s.empty();
        },
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
        [](auto&& s) {
            return s == "Hello World!\n";
        },
        [](auto&& s) {
            return s.empty();
        }
    },
    {{"-"}, "syntax error", 66,
        [](auto&& s) {
            return s.empty();
        },
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
        [](auto&& s) {
            return s.empty();
        }
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
        [](auto&& s) {
            return s.empty();
        }
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
        [](auto&& s) {
            return s.empty();
        }
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
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return s.empty();
        }
    },
    {{"-n", test::script_path("hello.ts")}, "", 0,
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return s.empty();
        }
    },
    {{"-n", "-"}, "syntax error", 66,
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                                    R"(Cannot parse -: 1:8: Expected '\(')")); 
        }
    },
    {{"-n", test::script_path("syntax_error.ts")}, "", 66,
        [](auto&& s) {
            return s.empty();
        },
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
        [](auto&& s) {
            return s == "Hello World!\n";
        },
        [](auto&& s) {
            return s.empty();
        }
    },
    {{"-s", "canon", test::script_path("hello.ts")}, "", 0,
        [](auto&& s) {
            return s == "Hello World!\n";
        },
        [](auto&& s) {
            return s.empty();
        }
    },
    {{"-s", "Undefined", "-"}, R"(print("Hello World!", "\n"))", 66,
        [](auto&& s) {
            return s.empty();
        },
        [](auto&& s) {
            return std::regex_search(s, std::regex(
                R"(Cannot parse -: Parse error: Unknown syntax "Undefined")")); 
        }
    },
    {{"-s", "Undefined", test::script_path("hello.ts")}, "", 66,
        [](auto&& s) {
            return s.empty();
        },
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
