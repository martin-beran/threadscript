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
namespace rules = tsp::rules;

using namespace std::string_literals;

namespace test {

struct parsed {
    parsed(std::string text, bool result, size_t line, size_t column,
           std::string error = "Parse error"):
        text(std::move(text)), result(result), line(line), column(column),
        error(std::move(error))
    {}
    std::string text;
    bool result;
    size_t line;
    size_t column;
    std::string error;
};

std::ostream& operator<<(std::ostream& os, const parsed& v)
{
    os << '"' << v.text << "\"->" << (v.result ? "OK" : "FAIL") <<
        ':' << v.line << ':' << v.column;
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

struct cnt12: parsed {
    cnt12(std::string text, bool result, size_t cnt1, size_t cnt2,
          size_t line, size_t column, std::string error = "Parse error"):
        parsed(std::move(text), result, line, column, std::move(error)),
        cnt1(cnt1), cnt2(cnt2)
    {}
    size_t cnt1;
    size_t cnt2;
};

std::ostream& operator<<(std::ostream& os, const cnt12& v)
{
    os << static_cast<const parsed&>(v) << " cnt1=" << v.cnt1 <<
        " cnt2=" << v.cnt2;
    return os;
}

} // namespace test
//! \endcond

/*! \file
 * \test \c fail -- test of threadscript::parser::rules::fail */
//! \cond
BOOST_AUTO_TEST_CASE(fail)
{
    std::string text = "abc";
    auto it = tsp::make_script_iterator(text);
    tsp::context ctx;
    rules::fail<decltype(ctx), tsp::empty, tsp::empty,
        typename decltype(it)::first_type> rule{};
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

/*! \file
 * \test \c eof -- test of threadscript::parser::rules::eof */
//! \cond
BOOST_DATA_TEST_CASE(eof, (std::vector<test::parsed>{
                               {"", true, 1, 1},
                               {"nonempty", false, 1, 1},
                           }))
{
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    rules::eof<decltype(ctx), tsp::empty, tsp::empty,
        typename decltype(it)::first_type> rule{};
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
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                BOOST_CHECK(e.pos() == begin);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
    }
}
//! \endcond

/*! \file
 * \test \c any -- test of threadscript::parser::rules::any */
//! \cond
BOOST_DATA_TEST_CASE(any, (std::vector<test::parsed>{
                               {"", false, 1, 1},
                               {"A", true, 1, 2},
                               {"nonempty", false, 1, 2, "Partial match"},
                               {"\n", true, 2, 1},
                           }))
{
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    char attr = '\0';
    rules::any<decltype(ctx), tsp::empty, tsp::empty,
        typename decltype(it)::first_type> rule{attr};
    if (sample.result) {
        auto pos = it.first;
        BOOST_REQUIRE_NO_THROW(
            try {
                pos = ctx.parse(rule, it);
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            } catch (...) {
                BOOST_TEST_INFO("unknown exception");
                throw;
            });
        BOOST_CHECK_EQUAL(attr, sample.text.front());
        BOOST_CHECK(pos == std::next(it.first));
        BOOST_CHECK_EQUAL(pos.line, sample.line);
        BOOST_CHECK_EQUAL(pos.column, sample.column);
    } else {
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it),
            tsp::error<typename decltype(it)::first_type>,
            ([it, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                if (sample.text.empty())
                    BOOST_CHECK(e.pos() == it.first); // empty input
                else
                    BOOST_CHECK(e.pos() == std::next(it.first));
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
        if (sample.text.empty())
            BOOST_CHECK_EQUAL(attr, '\0'); // unchanged
        else // failed after rules::any matched
            BOOST_CHECK_EQUAL(attr, sample.text.front());
    }
}
//! \endcond

/*! \file
 * \test \c t -- test of threadscript::parser::rules::t */
//! \cond
BOOST_DATA_TEST_CASE(t, (std::vector<test::parsed>{
                               {"", false, 1, 1},
                               {"A", true, 1, 2},
                               {"a", false, 1, 1},
                               {"?", false, 1, 1},
                               {"A nonempty", false, 1, 2, "Partial match"},
                         }))
{
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    char attr = '\0';
    rules::t<decltype(ctx), tsp::empty, tsp::empty,
        typename decltype(it)::first_type> rule{'A', attr};
    if (sample.result) {
        auto pos = it.first;
        BOOST_REQUIRE_NO_THROW(
            try {
                pos = ctx.parse(rule, it);
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
        BOOST_CHECK_EQUAL(attr, sample.text.front());
        BOOST_CHECK(pos == std::next(it.first));
        BOOST_CHECK_EQUAL(pos.line, sample.line);
        BOOST_CHECK_EQUAL(pos.column, sample.column);
    } else {
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it),
            tsp::error<typename decltype(it)::first_type>,
            ([it, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                if (sample.text.empty() || sample.text.front() != 'A')
                    BOOST_CHECK(e.pos() == it.first); // empty input
                else
                    BOOST_CHECK(e.pos() == std::next(it.first));
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
        if (sample.text.empty() || sample.text.front() != 'A')
            BOOST_CHECK_EQUAL(attr, '\0'); // unchanged
        else // failed after rules::t matched
            BOOST_CHECK_EQUAL(attr, sample.text.front());
    }
}
//! \endcond

/*! \file
 * \test \c repeat_0_1 -- test of threadscript::parser::rules::repeat,
 * \c threadscript::parser::operator-(), and
 * \c threadscript::parser::rule_base::operator[]() */
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
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    using ctx_t = decltype(ctx);
    using it_t = typename decltype(it)::first_type;
    using t = rules::t<ctx_t, tsp::empty, tsp::empty, it_t>;
    char c = '\0';
    size_t cnt = 100;
    auto set_c = [&c](auto&&, auto&&, auto&&, auto&&, auto&& it, auto&&) {
        c = *it;
    };
    auto set_cnt = [&cnt](auto&&, auto&&, auto&&, size_t n, auto&&, auto&&) {
        cnt = n;
    };
    auto rule = (-t{'a'}[set_c])[set_cnt];
    it_t pos;
    BOOST_REQUIRE_NO_THROW(
        try {
            pos = ctx.parse(rule, it, false);
        } catch (std::exception& e) {
            BOOST_TEST_INFO("exception: " << e.what());
            throw;
        });
    BOOST_CHECK_EQUAL(cnt, sample.cnt);
    if (sample.cnt == 0)
        BOOST_CHECK_EQUAL(c, '\0');
    else
        BOOST_CHECK_EQUAL(c, 'a');
    BOOST_CHECK_EQUAL(pos.line, sample.line);
    BOOST_CHECK_EQUAL(pos.column, sample.column);
}
//! \endcond

/*! \file
 * \test \c repeat_1_inf -- test of threadscript::parser::rules::repeat,
 * \c threadscript::parser::operator+(), and
 * \c threadscript::parser::rule_base::operator[]() */
//! \cond
BOOST_DATA_TEST_CASE(repeat_1_inf, (std::vector<test::repeated>{
                                        {"", false, 0, 1, 1},
                                        {"b", false, 0, 1, 1},
                                        {"ba", false, 0, 1, 1},
                                        {"a", true, 1, 1, 2},
                                        {"ab", true, 1, 1, 2},
                                        {"aa", true, 2, 1, 3},
                                        {"aaaaa", true, 5, 1, 6},
                                        {"aaaaab", true, 5, 1, 6},
                                        {"aaabaab", true, 3, 1, 4},
                                        {"aaa\naab", true, 3, 1, 4},
                                  }))
{
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    using ctx_t = decltype(ctx);
    using it_t = typename decltype(it)::first_type;
    using t = rules::t<ctx_t, tsp::empty, tsp::empty, it_t>;
    char c = '\0';
    size_t cnt = 100;
    auto set_c = [&c](auto&&, auto&&, auto&&, auto&&, auto&& it, auto&&) {
        c = *it;
    };
    auto set_cnt = [&cnt](auto&&, auto&&, auto&&, size_t n, auto&&, auto&&) {
        cnt = n;
    };
    auto rule = (+t{'a'}[set_c])[set_cnt];
    if (sample.result) {
        it_t pos;
        BOOST_REQUIRE_NO_THROW(
            try {
                pos = ctx.parse(rule, it, false);
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
        BOOST_CHECK_EQUAL(cnt, sample.cnt);
        if (sample.cnt == 0)
            BOOST_CHECK_EQUAL(c, '\0');
        else
            BOOST_CHECK_EQUAL(c, 'a');
        BOOST_CHECK_EQUAL(pos.line, sample.line);
        BOOST_CHECK_EQUAL(pos.column, sample.column);
    } else {
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it, false),
            tsp::error<typename decltype(it)::first_type>,
            ([it, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
        BOOST_CHECK_EQUAL(c, '\0'); // unchanged
        BOOST_CHECK_EQUAL(cnt, 100); // unchanged
    }
}
//! \endcond

/*! \file
 * \test \c repeat_0_inf -- test of threadscript::parser::rules::repeat,
 * \c threadscript::parser::operator*(), and
 * \c threadscript::parser::rule_base::operator[]() */
//! \cond
BOOST_DATA_TEST_CASE(repeat_0_inf, (std::vector<test::repeated>{
                                        {"", true, 0, 1, 1},
                                        {"b", true, 0, 1, 1},
                                        {"ba", true, 0, 1, 1},
                                        {"a", true, 1, 1, 2},
                                        {"ab", true, 1, 1, 2},
                                        {"aa", true, 2, 1, 3},
                                        {"aaaaa", true, 5, 1, 6},
                                        {"aaaaab", true, 5, 1, 6},
                                        {"aaabaab", true, 3, 1, 4},
                                        {"aaa\naab", true, 3, 1, 4},
                                  }))
{
    auto it = tsp::make_script_iterator(sample.text);
    tsp::context ctx;
    using ctx_t = decltype(ctx);
    using it_t = typename decltype(it)::first_type;
    using t = rules::t<ctx_t, tsp::empty, tsp::empty, it_t>;
    char c = '\0';
    size_t cnt = 100;
    auto set_c = [&c](auto&&, auto&&, auto&&, auto&&, auto&& it, auto&&) {
        c = *it;
    };
    auto set_cnt = [&cnt](auto&&, auto&&, auto&&, size_t n, auto&&, auto&&) {
        cnt = n;
    };
    auto rule = (*t{'a'}[set_c])[set_cnt];
    if (sample.result) {
        it_t pos;
        BOOST_REQUIRE_NO_THROW(
            try {
                pos = ctx.parse(rule, it, false);
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
        BOOST_CHECK_EQUAL(cnt, sample.cnt);
        if (sample.cnt == 0)
            BOOST_CHECK_EQUAL(c, '\0');
        else
            BOOST_CHECK_EQUAL(c, 'a');
        BOOST_CHECK_EQUAL(pos.line, sample.line);
        BOOST_CHECK_EQUAL(pos.column, sample.column);
    } else {
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it, false),
            tsp::error<typename decltype(it)::first_type>,
            ([it, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
        BOOST_CHECK_EQUAL(c, '\0'); // unchanged
        BOOST_CHECK_EQUAL(cnt, 100); // unchanged
    }
}
//! \endcond

/*! \file
 * \test \c repeat_child_rref -- tests that if an rvalue child is passed to
 * threadscript::parser::rules::repeat, then a copy is stored */
//! \cond
BOOST_AUTO_TEST_CASE(repeat_child_rref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    {
        auto a = any{};
        auto rule = -std::move(a);
        using rule_t = decltype(rule);
        static_assert(!std::is_reference_v<typename rule_t::child_type>);
        BOOST_TEST(&a != &rule.child());
    }
    {
        auto a = any{};
        auto rule = +std::move(a);
        using rule_t = decltype(rule);
        static_assert(!std::is_reference_v<typename rule_t::child_type>);
        BOOST_TEST(&a != &rule.child());
    }
    {
        auto a = any{};
        auto rule = *std::move(a);
        using rule_t = decltype(rule);
        static_assert(!std::is_reference_v<typename rule_t::child_type>);
        BOOST_TEST(&a != &rule.child());
    }
}
//! \endcond

/*! \file
 * \test \c repeat_child_lref -- tests that if an lvalue child is passed to
 * threadscript::parser::rules::repeat, then a reference is stored */
//! \cond
BOOST_AUTO_TEST_CASE(repeat_child_lref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    {
        auto a = any{};
        auto rule = -a;
        using rule_t = decltype(rule);
        static_assert(std::is_reference_v<typename rule_t::child_type>);
        BOOST_TEST(&a == &rule.child());
    }
    {
        auto a = any{};
        auto rule = +a;
        using rule_t = decltype(rule);
        static_assert(std::is_reference_v<typename rule_t::child_type>);
        BOOST_TEST(&a == &rule.child());
    }
    {
        auto a = any{};
        auto rule = *a;
        using rule_t = decltype(rule);
        static_assert(std::is_reference_v<typename rule_t::child_type>);
        BOOST_TEST(&a == &rule.child());
    }
}
//! \endcond

/*! \file
 * \test \c seq -- test of threadscript::parser::rules::seq,
 * \c threadscript::parser::operator>>(), and
 * \c threadscript::parser::rule_base::operator[]() */
//! \cond
BOOST_DATA_TEST_CASE(seq, (std::vector<test::repeated>{
                               {"", false, 0, 1, 1},
                               {"A", false, 1, 1, 2},
                               {"Ac", false, 2, 1, 3},
                               {"xyz", false, 0, 1, 1},
                               {"xcB", false, 0, 1, 1},
                               {"Acz", false, 2, 1, 3},
                               {"AcB", true, 3, 1, 4},
                               {"AcBx", false, 3, 1, 4, "Partial match"},
                           }))
{
    auto it = tsp::make_script_iterator(sample.text);
    using ctx_t = tsp::context;
    ctx_t ctx;
    using it_t = typename decltype(it)::first_type;
    using t = rules::t<ctx_t, tsp::empty, tsp::empty, it_t>;
    using any = rules::any<ctx_t, tsp::empty, tsp::empty, it_t>;
    char c = '\0';
    size_t cnt = 0;
    auto inc_cnt = [&cnt](auto&&, auto&&, auto&&, auto&&, auto&&, auto&&) {
        ++cnt;
    };
    auto rule = ((t{'A'}[inc_cnt] >> any{c})[inc_cnt] >> t{'B'})[inc_cnt];
    if (sample.result) {
        it_t pos;
        BOOST_REQUIRE_NO_THROW(
            try {
                pos = ctx.parse(rule, it);
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
        if (sample.cnt < 2)
            BOOST_CHECK_EQUAL(c, '\0');
        else
            BOOST_CHECK_EQUAL(c, 'c');
        BOOST_CHECK_EQUAL(pos.line, sample.line);
        BOOST_CHECK_EQUAL(pos.column, sample.column);
    } else {
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it),
            tsp::error<it_t>,
            ([it, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
    }
    BOOST_CHECK_EQUAL(cnt, sample.cnt);
}
//! \endcond

/*! \file
 * \test seq_child_rrref -- tests that if rvalue children are passed to
 * threadscript::parser::rules::seq, then copies are stored */
//! \cond
BOOST_AUTO_TEST_CASE(seq_child_rrref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    auto a1 = any{};
    auto a2 = any{};
    auto rule = std::move(a1) >> std::move(a2);
    using rule_t = decltype(rule);
    static_assert(!std::is_reference_v<typename rule_t::child1_type>);
    static_assert(!std::is_reference_v<typename rule_t::child2_type>);
    BOOST_TEST(&a1 != &rule.child1());
    BOOST_TEST(&a2 != &rule.child2());
}
//! \endcond

/*! \file
 * \test seq_child_lrref -- tests that if lvalue and rvalue children are passed
 * to threadscript::parser::rules::seq, then a reference and a copy are stored
 */
//! \cond
BOOST_AUTO_TEST_CASE(seq_child_lrref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    auto a1 = any{};
    auto a2 = any{};
    auto rule = a1 >> std::move(a2);
    using rule_t = decltype(rule);
    static_assert(std::is_reference_v<typename rule_t::child1_type>);
    static_assert(!std::is_reference_v<typename rule_t::child2_type>);
    BOOST_TEST(&a1 == &rule.child1());
    BOOST_TEST(&a2 != &rule.child2());
}
//! \endcond

/*! \file
 * \test seq_child_rlref -- tests that if rvalue and lvalue children are passed
 * to threadscript::parser::rules::seq, then a copy and a reference are stored
 */
//! \cond
BOOST_AUTO_TEST_CASE(seq_child_rlref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    auto a1 = any{};
    auto a2 = any{};
    auto rule = std::move(a1) >> a2;
    using rule_t = decltype(rule);
    static_assert(!std::is_reference_v<typename rule_t::child1_type>);
    static_assert(std::is_reference_v<typename rule_t::child2_type>);
    BOOST_TEST(&a1 != &rule.child1());
    BOOST_TEST(&a2 == &rule.child2());
}
//! \endcond

/*! \file
 * \test seq_child_llref -- tests that if lvalue and lvalue children are passed
 * to threadscript::parser::rules::seq, then references are stored */
//! \cond
BOOST_AUTO_TEST_CASE(seq_child_llref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    auto a1 = any{};
    auto a2 = any{};
    auto rule = a1 >> a2;
    using rule_t = decltype(rule);
    static_assert(std::is_reference_v<typename rule_t::child1_type>);
    static_assert(std::is_reference_v<typename rule_t::child2_type>);
    BOOST_TEST(&a1 == &rule.child1());
    BOOST_TEST(&a2 == &rule.child2());
}
//! \endcond

/*! \file
 * \test \c alt -- test of threadscript::parser::rules::alt,
 * \c threadscript::parser::operator|(), and
 * \c threadscript::parser::rule_base::operator[]() */
//! \cond
BOOST_DATA_TEST_CASE(alt, (std::vector<test::cnt12>{
                               {"", false, 0, 0, 1, 1},
                               {"x", false, 0, 0, 1, 1},
                               {"A", true, 1, 1, 1, 2},
                               {"B", true, 2, 1, 1, 2},
                               {"C", true, 0, 2, 1, 2},
                               {"Aa", false, 1, 1, 1, 2, "Partial match"},
                               {"Ba", false, 2, 1, 1, 2, "Partial match"},
                               {"Ca", false, 0, 2, 1, 2, "Partial match"},
                           }))
{
    auto it = tsp::make_script_iterator(sample.text);
    using ctx_t = tsp::context;
    ctx_t ctx;
    using it_t = typename decltype(it)::first_type;
    using t = rules::t<ctx_t, tsp::empty, tsp::empty, it_t>;
    char c = '\0';
    size_t cnt1 = 0;
    size_t cnt2 = 0;
    auto set_cnt1 =
        [&c, &cnt1](auto&&, auto&&, auto&&, size_t n, auto b, auto e) {
            cnt1 = n;
            BOOST_REQUIRE(b != e);
            c = *b;
        };
    auto set_cnt2 =
        [&c, &cnt2](auto&&, auto&&, auto&&, size_t n, auto b, auto e) {
            cnt2 = n;
            BOOST_REQUIRE(b != e);
            c = *b;
        };
    auto rule = ((t{'A'} | t{'B'})[set_cnt1] | t{'C'})[set_cnt2];
    if (sample.result) {
        it_t pos;
        BOOST_REQUIRE_NO_THROW(
            try {
                pos = ctx.parse(rule, it);
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
        switch (sample.cnt1) {
        case 0:
            switch (sample.cnt2) {
            case 0:
                BOOST_CHECK_EQUAL(c, '\0');
                break;
            case 1:
                BOOST_CHECK(false);
                break;
            case 2:
                BOOST_CHECK_EQUAL(c, 'C');
                break;
            default:
                BOOST_CHECK(false);
                break;
            }
            break;
        case 1:
            BOOST_CHECK_EQUAL(c, 'A');
            break;
        case 2:
            BOOST_CHECK_EQUAL(c, 'B');
            break;
        default:
            BOOST_CHECK(false);
            break;
        }
        BOOST_CHECK_EQUAL(pos.line, sample.line);
        BOOST_CHECK_EQUAL(pos.column, sample.column);
    } else {
        BOOST_CHECK_EXCEPTION(ctx.parse(rule, it),
            tsp::error<it_t>,
            ([it, &sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
    }
    BOOST_CHECK_EQUAL(cnt1, sample.cnt1);
    BOOST_CHECK_EQUAL(cnt2, sample.cnt2);
}
//! \endcond

/*! \file
 * \test alt_child_rrref -- tests that if rvalue children are passed to
 * threadscript::parser::rules::alt, then copies are stored */
//! \cond
BOOST_AUTO_TEST_CASE(alt_child_rrref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    auto a1 = any{};
    auto a2 = any{};
    auto rule = std::move(a1) | std::move(a2);
    using rule_t = decltype(rule);
    static_assert(!std::is_reference_v<typename rule_t::child1_type>);
    static_assert(!std::is_reference_v<typename rule_t::child2_type>);
    BOOST_TEST(&a1 != &rule.child1());
    BOOST_TEST(&a2 != &rule.child2());
}
//! \endcond

/*! \file
 * \test alt_child_lrref -- tests that if lvalue and rvalue children are passed
 * to threadscript::parser::rules::alt, then a reference and a copy are stored
 */
//! \cond
BOOST_AUTO_TEST_CASE(alt_child_lrref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    auto a1 = any{};
    auto a2 = any{};
    auto rule = a1 | std::move(a2);
    using rule_t = decltype(rule);
    static_assert(std::is_reference_v<typename rule_t::child1_type>);
    static_assert(!std::is_reference_v<typename rule_t::child2_type>);
    BOOST_TEST(&a1 == &rule.child1());
    BOOST_TEST(&a2 != &rule.child2());
}
//! \endcond

/*! \file
 * \test alt_child_rlref -- tests that if rvalue and lvalue children are passed
 * to threadscript::parser::rules::alt, then a copy and a reference are stored
 */
//! \cond
BOOST_AUTO_TEST_CASE(alt_child_rlref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    auto a1 = any{};
    auto a2 = any{};
    auto rule = std::move(a1) | a2;
    using rule_t = decltype(rule);
    static_assert(!std::is_reference_v<typename rule_t::child1_type>);
    static_assert(std::is_reference_v<typename rule_t::child2_type>);
    BOOST_TEST(&a1 != &rule.child1());
    BOOST_TEST(&a2 == &rule.child2());
}
//! \endcond

/*! \file
 * \test alt_child_llref -- tests that if lvalue and lvalue children are passed
 * to threadscript::parser::rules::alt, then references are stored */
//! \cond
BOOST_AUTO_TEST_CASE(alt_child_llref)
{
    using it_t = std::string::iterator;
    using any = rules::any<tsp::context, tsp::empty, tsp::empty, it_t>;
    auto a1 = any{};
    auto a2 = any{};
    auto rule = a1 | a2;
    using rule_t = decltype(rule);
    static_assert(std::is_reference_v<typename rule_t::child1_type>);
    static_assert(std::is_reference_v<typename rule_t::child2_type>);
    BOOST_TEST(&a1 == &rule.child1());
    BOOST_TEST(&a2 == &rule.child2());
}
//! \endcond
