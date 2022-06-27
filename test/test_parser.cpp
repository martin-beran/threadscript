/*! \file
 * \brief Tests of namespace threadscript::parser
 */

//! \cond
#include "threadscript/parser.hpp"

#define BOOST_TEST_MODULE parser
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace ts = threadscript;
namespace tsp = ts::parser;

using namespace std::string_literals;

namespace test {

struct parsed {
    std::string text;
    bool result;
    size_t line;
    size_t column;
};

std::ostream& operator<<(std::ostream& os, const parsed& v)
{
    os << '"' << v.text << "\"->" << (v.result ? "OK" : "FAIL") <<
        ':' << v.line << ':' << v.column;
    return os;
}

} // namespace test
//! \endcond

//! \cond
BOOST_AUTO_TEST_CASE(fail)
{
    std::string text = "abc";
    auto it = tsp::make_script_iterator(text);
    tsp::context ctx;
    tsp::rules::fail<decltype(ctx), typename decltype(it)::first_type> rule{};
    auto begin = it.first;
    BOOST_CHECK_EXCEPTION(ctx.parse(rule, it),
        tsp::error<decltype(it)::first_type>,
        [begin](auto&& e) {
            BOOST_CHECK_EQUAL(e.what(), "Parse error"s);
            BOOST_CHECK(e.pos() == begin);
            BOOST_CHECK_EQUAL(e.pos().line, 1);
            BOOST_CHECK_EQUAL(e.pos().column, 1);
            return true;
        });
}
//! \endcond

//! \cond
BOOST_DATA_TEST_CASE(eof, (std::vector<test::parsed>{
                               {"", true, 1, 1},
                               {"nonempty", false, 1, 1},
                           }))
{
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    tsp::rules::eof<decltype(ctx), typename decltype(it)::first_type> rule{};
    auto begin = it.first;
    if (sample.result) {
        BOOST_CHECK_NO_THROW(ctx.parse(rule, it));
        BOOST_CHECK(begin == it.first);
        BOOST_CHECK_EQUAL(begin.line, sample.line);
        BOOST_CHECK_EQUAL(begin.column, sample.column);
    } else {
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it),
            tsp::error<typename decltype(it)::first_type>,
            ([begin, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), "Parse error"s);
                BOOST_CHECK(e.pos() == begin);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
    }
}
//! \endcond
//
//! \cond
BOOST_DATA_TEST_CASE(t, (std::vector<test::parsed>{
                               {"", false, 1, 1},
                               {"nonempty", true, 1, 2},
                               {"\n", true, 2, 1},
                           }))
{
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    char attr = '\0';
    tsp::rules::t<decltype(ctx), typename decltype(it)::first_type> rule{attr};
    auto begin = it.first;
    if (sample.result) {
        BOOST_CHECK_NO_THROW(ctx.parse(rule, it));
        BOOST_CHECK_EQUAL(attr, sample.text.front());
        BOOST_CHECK(std::next(begin) == it.first);
        BOOST_CHECK_EQUAL(it.first.line, sample.line);
        BOOST_CHECK_EQUAL(it.first.column, sample.column);
    } else {
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it),
            tsp::error<typename decltype(it)::first_type>,
            ([begin, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), "Parse error"s);
                BOOST_CHECK(e.pos() == begin);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
    }
}
//! \endcond
