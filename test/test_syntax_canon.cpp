/*! \file
 * \brief Tests of class threadscript::syntax::canon
 */

//! \cond
#include "threadscript/code_parser.hpp"

#define BOOST_TEST_MODULE parser
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace ts = threadscript;

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

void test_trace(std::optional<ts::parser::rule_result> result,
                const std::string& name, size_t depth,
                size_t begin_line, size_t begin_column,
                size_t end_line, size_t end_column)
{
    BOOST_TEST_MESSAGE(ts::parser::context::trace_msg(result, name, depth,
                                                      begin_line, begin_column,
                                                      end_line, end_column));
}

template <class Sample> void test_parse(Sample&& sample)
{
    test_parse(std::forward<Sample>(sample), [](){});
}

void test_parse(auto&& sample, auto&& check)
{
    ts::allocator_any alloc;
    if (sample.result)
        BOOST_REQUIRE_NO_THROW(
            try {
                auto parsed = ts::parse_code(alloc, sample.text, "string",
                                             ts::syntax_factory::syntax_canon,
                                             test_trace);
                BOOST_CHECK_NE(parsed.get(), nullptr);
                check();
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
    else
        BOOST_CHECK_EXCEPTION(ts::parse_code(alloc, sample.text, "string",
                                             ts::syntax_factory::syntax_canon,
                                             test_trace),
            ts::parse_error,
            ([&sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
}

} // namespace test
//! \endcond

/*! \file
 * \test \c dummy -- A dummy test, to be removed and replaced by real tests */
//! \cond
BOOST_DATA_TEST_CASE(parse, (std::vector<test::parsed>{
                                 {"", false, 1, 1},
                     }))
{
    test_parse(sample);
}
//! \endcond
