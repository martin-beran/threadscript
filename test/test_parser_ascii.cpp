/*! \file
 * \brief Tests of namespace threadscript::parser_ascii
 */

//! \cond
#include "threadscript/parser.hpp"
#include "threadscript/parser_ascii.hpp"
#include <cctype>
#include <limits>
#include <type_traits>

#define BOOST_TEST_MODULE parser
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace ts = threadscript;
namespace tsp = ts::parser;
//namespace tsr = tsp::rules;
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

struct repeated: parsed {
    repeated(std::string text, bool result, size_t cnt, size_t line,
             size_t column, std::string error = "Parse error"):
        parsed(std::move(text), result, line, column, std::move(error)),
        cnt(cnt)
    {}
    size_t cnt;
};

std::ostream& operator<<(std::ostream& os, const repeated& v)
{
    os << static_cast<const parsed&>(v) << " cnt=" << v.cnt;
    return os;
}

template <class Sample, class Rule>
void test_parse(Sample&& sample, Rule&& rule, bool all = true)
{
    test_parse(std::forward<Sample>(sample), std::forward<Rule>(rule), all,
               [](){});
}

void test_parse(auto&& sample, auto&& rule, bool all, auto&& check)
{
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    using it_t = typename decltype(it)::first_type;
    BOOST_TEST_MESSAGE("result=" << sample.result);
    if (sample.result)
        BOOST_REQUIRE_NO_THROW(
            try {
                auto pos = ctx.parse(rule, it, all);
                BOOST_CHECK_EQUAL(pos.line, sample.line);
                BOOST_CHECK_EQUAL(pos.column, sample.column);
                check();
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
    else
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it),
            tsp::error<it_t>,
            ([it, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
}

} // namespace test
//! \endcond

/* \file
 * \test \c to_lower -- test of threadscript::parser_ascii::to_lower() */
//! \cond
BOOST_AUTO_TEST_CASE(to_lower)
{
    std::array<char, 256> chars{};
    for (int i = 0; i < 256; ++i)
        chars[i] = char(i);
    char c = 'a';
    for (int i = 'A'; i <= 'Z'; ++i)
        chars[i] = char(c++);
    for (int i = 0; i < 256; ++i)
        BOOST_CHECK_EQUAL(tspa::to_lower(char(i)), chars[i]);
}
//! \endcond

/* \file
 * \test \c to_upper -- test of threadscript::parser_ascii::to_upper() */
//! \cond
BOOST_AUTO_TEST_CASE(to_upper)
{
    std::array<char, 256> chars{};
    for (int i = 0; i < 256; ++i)
        chars[i] = char(i);
    char c = 'A';
    for (int i = 'a'; i <= 'z'; ++i)
        chars[i] = char(c++);
    for (int i = 0; i < 256; ++i)
        BOOST_CHECK_EQUAL(tspa::to_upper(char(i)), chars[i]);
}
//! \endcond

/*! \file
 * \test \c equal_ic -- test of threadscript::parser_ascii::equal_ic */
//! \cond
BOOST_AUTO_TEST_CASE(equal_ic)
{
    for (int a = 0; a < 256; ++a)
        for (int b = 0; b < 256; ++b) {
            BOOST_TEST_INFO("a=" << a << " b=" << b);
            bool res = a == b;
            if (a >= 'a' && a <= 'z')
                res = res || tspa::to_upper(char(a)) == b;
            if (b >= 'a' && b <= 'z')
                res = res || a == tspa::to_upper(char(b));
            BOOST_CHECK_EQUAL(tspa::equal_ic()(a, b), res);
        }
}
//! \endcond

/*! \file
 * \test \c fail -- test of threadscript::parser_ascii::rules::factory::fail */
//! \cond
BOOST_DATA_TEST_CASE(fail, (std::vector<test::parsed>{
                                {"", false, 1, 1},
                                {"nonempty", false, 1, 1},
                     }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::fail();
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c eof -- test of threadscript::parser_ascii::rules::factory::eof */
//! \cond
BOOST_DATA_TEST_CASE(eof, (std::vector<test::parsed>{
                               {"", true, 1, 1},
                               {"nonempty", false, 1, 1},
                     }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::eof();
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c any -- test of threadscript::parser_ascii::rules::factory::any */
//! \cond
BOOST_DATA_TEST_CASE(any, (std::vector<test::parsed>{
                               {"", false, 1, 1},
                               {"A", true, 1, 2},
                               {"nonempty", false, 1, 2, "Partial match"},
                               {"\n", true, 2, 1},
                     }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::any();
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c t -- test of threadscript::parser_ascii::rules::factory::t */
//! \cond
BOOST_DATA_TEST_CASE(t, (std::vector<test::parsed>{
                             {"", false, 1, 1},
                             {"A", true, 1, 2},
                             {"a", false, 1, 1},
                             {"?", false, 1, 1},
                             {"A nonempty", false, 1, 2, "Partial match"},
                     }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::t('A');
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c p -- test of threadscript::parser_ascii::rules::factory::p */
//! \cond
BOOST_DATA_TEST_CASE(p, (std::vector<test::parsed>{
                             {"", false, 1, 1},
                             {"A", true, 1, 2},
                             {"B", true, 1, 2},
                             {"a", false, 1, 1},
                             {"?", false, 1, 1},
                             {"A nonempty", false, 1, 2, "Partial match"},
                             {"Z nonempty", false, 1, 2, "Partial match"},
                     }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::p(
        [](const char& c) {
            return c >= 'A' && c <= 'Z';
        });
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c str_string -- test of
 * threadscript::parser_ascii::rules::factory::str matching \c std::string */
//! \cond
BOOST_DATA_TEST_CASE(str_string, (std::vector<test::parsed>{
                                {"", false, 1, 1},
                                {"A", false, 1, 2},
                                {"AB", false, 1, 3},
                                {"ABC", true, 1, 4},
                                {"aBC", false, 1, 1},
                                {"AbC", false, 1, 2},
                                {"ABc", false, 1, 3},
                                {"ABC nonempty", false, 1, 4, "Partial match"},
                                }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::str("ABC"s);
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c str_ic_string -- test of
 * threadscript::parser_ascii::rules::factory::str_ic matching \c std::string */
//! \cond
BOOST_DATA_TEST_CASE(str_ic_string, (std::vector<test::parsed>{
                                {"", false, 1, 1},
                                {"A", false, 1, 2},
                                {"AB", false, 1, 3},
                                {"ABC", true, 1, 4},
                                {"aBC", true, 1, 4},
                                {"AbC", true, 1, 4},
                                {"ABc", true, 1, 4},
                                {"ABC nonempty", false, 1, 4, "Partial match"},
                                }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::str_ic("ABC"s);
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c str_string_view -- test of
 * threadscript::parser_ascii::rules::factory::str matching
 * \c std::string_view */
//! \cond
BOOST_DATA_TEST_CASE(str_string_view, (std::vector<test::parsed>{
                                {"", false, 1, 1},
                                {"A", false, 1, 2},
                                {"AB", false, 1, 3},
                                {"ABC", true, 1, 4},
                                {"aBC", false, 1, 1},
                                {"AbC", false, 1, 2},
                                {"ABc", false, 1, 3},
                                {"ABC nonempty", false, 1, 4, "Partial match"},
                                }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::str("ABC"sv);
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c str_ic_string_view -- test of
 * threadscript::parser_ascii::rules::factory::str_ic matching
 * \c std::string_view */
//! \cond
BOOST_DATA_TEST_CASE(str_ic_string_view, (std::vector<test::parsed>{
                                {"", false, 1, 1},
                                {"A", false, 1, 2},
                                {"AB", false, 1, 3},
                                {"ABC", true, 1, 4},
                                {"aBC", true, 1, 4},
                                {"AbC", true, 1, 4},
                                {"ABc", true, 1, 4},
                                {"ABC nonempty", false, 1, 4, "Partial match"},
                                }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    auto rule = f::str_ic("ABC"sv);
    test_parse(sample, rule);
}
//! \endcond

/*! \file
 * \test \c repeat_0_1 -- test of threadscript::parser::rules::repeat,
 * \c threadscript::parser::operator-(), and
 * \c threadscript::parser::rule_base::operator[](). This test verifies that
 * some operators from threadscript::parser are usable with rules created by
 * threadscript::parser_ascii::rules::factory. Based on this, we expect that
 * other operators work as well, because they are tested in test_parser.cpp and
 * a problem related to the factory would also occur for operators tested
 * here. */
//! \cond
BOOST_DATA_TEST_CASE(repeat_0_1, (std::vector<test::repeated>{
                                      {"", true, 0, 1, 1},
                                      {"b", true, 0, 1, 1},
                                      {"ba", true, 0, 1, 1},
                                      {"a", true, 1, 1, 2},
                                      {"ab", true, 1, 1, 2},
                                      {"aa", true, 1, 1, 2},
                                  }))
{
    using f = tsra::factory<tsp::script_iterator<
        typename decltype(sample.text)::const_iterator>>;
    char c = '\0';
    size_t cnt = 100;
    auto set_c = [&c](auto&&, auto&&, auto&&, auto&&, auto&& it, auto&&) {
        c = *it;
    };
    auto set_cnt = [&cnt](auto&&, auto&&, auto&&, auto&& n, auto&&, auto&&) {
        cnt = n;
    };
    auto rule = (-f::t('a')[set_c])[set_cnt];
    test_parse(sample, rule, false,
               [&c, &cnt, &sample]() {
                   if (sample.cnt == 0)
                       BOOST_CHECK_EQUAL(c, '\0');
                   else
                       BOOST_CHECK_EQUAL(c, 'a');
                   BOOST_CHECK_EQUAL(cnt, sample.cnt);
               });
}
//! \endcond
