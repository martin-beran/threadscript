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
 * \test \c script_file -- Program \link ts.cpp ts\endlink with a script file */
//! \cond
BOOST_DATA_TEST_CASE(script_file, (std::vector<test::ts_result>{
    {{test::script_path("hello.ts")}, "", 0,
        [](auto&& s) {
            return s == "Hello World!\n";
            return s.empty();
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
 * \test \c script_stdin -- Program \link ts.cpp ts\endlink with a script on
 * standard input */
//! \cond
BOOST_DATA_TEST_CASE(script_stdin, (std::vector<test::ts_result>{
    {{"-"}, R"(print("Hello World!", "\n"))", 0,
        [](auto&& s) {
            return s == "Hello World!\n";
            return s.empty();
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
