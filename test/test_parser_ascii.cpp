/*! \file
 * \brief Tests of namespace threadscript::parser_ascii
 */

//! \cond
#include "threadscript/parser_ascii.hpp"
#include <cctype>

#define BOOST_TEST_MODULE parser
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace ts = threadscript;
namespace tsp = ts::parser;
namespace tsr = tsp::rules;
namespace tspa = ts::parser_ascii;
namespace tsra = tspa::rules;

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace test {

struct input {
    input(std::string text, size_t line, size_t column,
          std::string error = "Parse error"):
        text(std::move(text)), line(line), column(column),
        error(std::move(error))
    {}
    std::string text;
    size_t line;
    size_t column;
    std::string error;
};

std::ostream& operator<<(std::ostream& os, const input& v)
{
    os << '"' << v.text << "\"->" << v.line << ':' << v.column;
    return os;
}

struct parsed: input {
    parsed(std::string text, bool result, size_t line, size_t column,
           std::string error = "Parse error"):
        input(std::move(text), line, column, std::move(error)), result(result)
    {}
    bool result;
};

std::ostream& operator<<(std::ostream& os, const parsed& v)
{
    os << static_cast<const input&>(v) << ':' << (v.result ? "OK" : "FAIL");
    return os;
}

} // namespace test
//! \endcond

/* \file
 * \test \c dummy -- a dummy test (to be removed) of
 * threadscript::parser_ascii::rules::... */
//! \cond
BOOST_AUTO_TEST_CASE(dummy)
{
}
//! \endcond
