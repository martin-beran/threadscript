/*! \file
 * \brief Tests of predefined built-in (C++ native) symbols
 *
 * It tests contents of namespace threadscript::predef. Tests should be kept in
 * the order of declarations of tested classes in the namespace (in file
 * predef.hpp).
 */

//! \cond
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE predef
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <sstream>
#include <variant>

//! [parse_run]
namespace ts = threadscript;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace test {

ts::allocator_any alloc;

struct script_runner {
    script_runner(std::string script): script(std::move(script)) {
        // Redirect standard output to a string stream
        vm.std_out = &std_out;
        // Register default predefined built-in native commands and functions
        vm.sh_vars = ts::predef_symbols(alloc);
    }
    ts::value::value_ptr run();
    std::ostringstream std_out;
    ts::virtual_machine vm{alloc};
    std::string script;
};

ts::value::value_ptr script_runner::run()
{
    // Parse the script
    auto parsed = ts::parse_code(alloc, script, "string");
    // Prepare script runtime environment
    ts::state thread{vm};
    // Run the script
    auto result = parsed->eval(thread);
    return result;
}

} // namespace test
//! [parse_run]

namespace test {

using int_t = ts::config::value_int_type;
using uint_t = ts::config::value_unsigned_type;

struct exc {
    const std::type_info& type;
    ts::frame_location location;
    std::string_view msg;
};

struct runner_result {
    template <class T> runner_result(std::string script, T result,
                                     std::string std_out):
        script(std::move(script)), result(result), std_out(std::move(std_out))
    {}
    runner_result(std::string script, const char* result, std::string std_out):
        script(std::move(script)), result(std::string_view(result)),
        std_out(std::move(std_out))
    {}
    std::string script;
    std::variant<std::nullptr_t, bool, int_t, uint_t, std::string_view, exc>
        result;
    std::string std_out;
};

std::ostream& operator<<(std::ostream& os, const runner_result& rr)
{
    os << rr.script;
    return os;
}

template <class R, class E>
void check_value(ts::value::value_ptr result, E* expected)
{
    BOOST_REQUIRE(result);
    auto pr = dynamic_cast<R*>(result.get());
    BOOST_TEST_INFO("Unexpected value of type: " << result->type_name());
    BOOST_REQUIRE(pr);
    BOOST_CHECK_EQUAL(pr->cvalue(), *expected);
}

void check_runner(const runner_result& sample)
{
    test::script_runner runner(sample.script);
    if (auto ps = std::get_if<test::exc>(&sample.result)) {
        BOOST_CHECK_EXCEPTION(runner.run(), ts::exception::base,
            ([&sample, &ps](auto&& e) {
                BOOST_CHECK(typeid(e) == ps->type);
                BOOST_CHECK_EQUAL(e.location().function,
                                  ps->location.function);
                BOOST_CHECK_EQUAL(e.location().file, "string");
                BOOST_CHECK_EQUAL(e.location().line, ps->location.line);
                BOOST_CHECK_EQUAL(e.location().column, ps->location.column);
                BOOST_CHECK_EQUAL(e.msg(), ps->msg);
                return true;
            }));
    } else {
        ts::value::value_ptr result;
        BOOST_REQUIRE_NO_THROW(
            try {
                result = runner.run();
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
        if (std::get_if<std::nullptr_t>(&sample.result)) {
            if (result)
                BOOST_TEST_INFO("Unexpected value of type: " << result->type_name());
            BOOST_CHECK(!result);
        } else {
            if (auto ps = std::get_if<bool>(&sample.result)) {
                check_value<ts::value_bool>(result, ps);
            } else if (auto ps = std::get_if<int_t>(&sample.result)) {
                check_value<ts::value_int>(result, ps);
            } else if (auto ps = std::get_if<uint_t>(&sample.result)) {
                check_value<ts::value_unsigned>(result, ps);
            } else if (auto ps = std::get_if<std::string_view>(&sample.result)) {
                check_value<ts::value_string>(result, ps);
            }
        }
    }
    BOOST_CHECK_EQUAL(runner.std_out.view(), sample.std_out);
}

constexpr auto u_max = std::numeric_limits<uint_t>::max();
constexpr auto u_half = u_max / 2;
constexpr auto i_min = std::numeric_limits<int_t>::min();
constexpr auto i_n_half = i_min / 2; // negative
constexpr auto i_max = std::numeric_limits<int_t>::max();
constexpr auto i_p_half = i_max / 2; // positive

std::string u_op(const std::string op, uint_t a, uint_t b)
{
    return op + "(" + std::to_string(a) + ", " + std::to_string(b) + ")";
}

std::string u_op(const std::string op, uint_t a)
{
    return op + "(" + std::to_string(a) + ")";
}

std::string i_op(const std::string op, int_t a, int_t b)
{
    return op + "(" + (a >= 0 ? "+" : "") +  std::to_string(a) + ", " +
        (b >= 0 ? "+" : "") + std::to_string(b) + ")";
}

std::string i_op(const std::string op, int_t a)
{
    return op + "(" + (a >= 0 ? "+" : "") +  std::to_string(a) + ")";
}

} // namespace test
//! \endcond

/*! \file
 * \test \c f_add -- Test of threadscript::predef::f_add */
//! \cond
BOOST_DATA_TEST_CASE(f_add, (std::vector<test::runner_result>{
    {R"(add())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(add(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(add(null, 2, 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(add(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(add(null, null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(add(1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(add(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(add(false, true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(add(1, +2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(add(-1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(add(1, "a"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(add("b", -2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {test::u_op("add", 0, 0), test::uint_t(0), ""},
    {test::u_op("add", 12, 34), test::uint_t(46), ""},
    {test::u_op("add", test::u_half, test::u_half),
        test::uint_t(test::u_max - 1U), ""},
    {test::u_op("add", test::u_half + 1U, test::u_half),
        test::uint_t(test::u_max), ""},
    {test::u_op("add", test::u_half, test::u_half + 2U), test::uint_t(0), ""},
    {test::u_op("add", test::u_half + 1U, test::u_half + 2U),
        test::uint_t(1U), ""},
    {test::u_op("add", test::u_max, 0U), test::uint_t(test::u_max), ""},
    {test::u_op("add", test::u_max, 1U), test::uint_t(0U), ""},
    {test::u_op("add", test::u_max, test::u_half),
        test::uint_t(test::u_half - 1U), ""},
    {test::i_op("add", 0, 0), test::int_t(0), ""},
    {test::i_op("add", 12, 34), test::int_t(46), ""},
    {test::i_op("add", -12, 34), test::int_t(22), ""},
    {test::i_op("add", 12, -34), test::int_t(-22), ""},
    {test::i_op("add", -12, -34), test::int_t(-46), ""},
    {test::i_op("add", test::i_p_half, test::i_p_half),
        test::int_t(test::i_max - 1), ""},
    {test::i_op("add", test::i_p_half + 1, test::i_p_half),
        test::int_t(test::i_max), ""},
    {test::i_op("add", test::i_p_half, test::i_p_half + 2), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("add", test::i_p_half + 1, test::i_p_half + 2), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("add", test::i_n_half, test::i_n_half),
        test::int_t(test::i_min), ""},
    {test::i_op("add", test::i_n_half, test::i_n_half - 1), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("add", test::i_n_half - 2, test::i_n_half), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("add", test::i_max, 0), test::int_t(test::i_max), ""},
    {test::i_op("add", 0, test::i_max), test::int_t(test::i_max), ""},
    {test::i_op("add", test::i_max, 1), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("add", -1, test::i_min), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {R"(add("", ""))", "", ""},
    {R"(add("A", ""))", "A", ""},
    {R"(add("ABC", "xy"))", "ABCxy", ""},
    {R"(add(0, 1, 2))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(add(null, 1, 2))", test::uint_t(3), ""}, // target is null
    {R"(add(true, 1, 1))", test::uint_t(2), ""}, // target constant, wrong type
    // target is modifiable
    {R"(
        seq(
            var("r", clone(0)),
            print(is_same(add(var("r"), 2, 3), var("r"))),
            var("r")
        )
    )", test::uint_t(5), "true"},
    {R"(
        seq(
            var("r", clone(+0)),
            print(is_same(add(var("r"), -1, +3), var("r"))),
            var("r")
        )
    )", test::int_t(2), "true"},
    {R"(
        seq(
            var("r", clone("")),
            print(is_same(add(var("r"), "Hello ", "World!"), var("r"))),
            var("r")
        )
    )", "Hello World!", "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_and -- Test of threadscript::predef::f_and, which also applies to
 * threadscript::predef::f_and_base (the part of implementation shared with
 * threadscript::predef::f_and_r) */
//! \cond
BOOST_DATA_TEST_CASE(f_and, (std::vector<test::runner_result>{
    {R"(and(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(and(false, null))", false, ""},
    {R"(and(true, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(and(true, null, false))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(and())", true, ""},
    {R"(and(seq(print(1), false)))", false, "1"},
    {R"(and(seq(print(1), true)))", true, "1"},
    {R"(and(1))", true, ""},
    {R"(and(-1))", true, ""},
    {R"(and("str"))", true, ""},
    {R"(and(seq(print(1), false), seq(print(2), false)))", false, "1"},
    {R"(and(seq(print(1), false), seq(print(2), true)))", false, "1"},
    {R"(and(seq(print(1), true), seq(print(2), false)))", false, "12"},
    {R"(and(seq(print(1), true), seq(print(2), true)))", true, "12"},
    {R"(and(seq(print(1), false), seq(print(2), false), seq(print(3), false)))",
        false, "1"},
    {R"(and(seq(print(1), false), seq(print(2), false), seq(print(3), true)))",
        false, "1"},
    {R"(and(seq(print(1), false), seq(print(2), true), seq(print(3), false)))",
        false, "1"},
    {R"(and(seq(print(1), false), seq(print(2), true), seq(print(3), true)))",
        false, "1"},
    {R"(and(seq(print(1), true), seq(print(2), false), seq(print(3), false)))",
        false, "12"},
    {R"(and(seq(print(1), true), seq(print(2), false), seq(print(3), true)))",
        false, "12"},
    {R"(and(seq(print(1), true), seq(print(2), true), seq(print(3), false)))",
        false, "123"},
    {R"(and(seq(print(1), true), seq(print(2), true), seq(print(3), true)))",
        true, "123"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_and_r -- Test of threadscript::predef::f_and_r. Almost all
 * implementation of f_and_r is shared with f_and, therefore we do only a small
 * number of checks here, assuming that tests of f_and apply here, too. */
//! \cond
BOOST_DATA_TEST_CASE(f_and_r, (std::vector<test::runner_result>{
    {R"(and_r())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(and_r(false, true, false))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(and_r(null, true, false))", false, ""}, // target is null
    {R"(and_r(1, true, false))", false, ""}, // target is constant, but a
                                               // wrong type
    {R"(and_r(clone(true), true, false))", false, ""}, // target is modifiable
    {R"(and_r(clone(false), true, true))", true, ""},
    {R"(and_r(clone(false)))", true, ""},
    {R"(and_r(clone(false), false))", false, ""},
    {R"(and_r(clone(false), true))", true, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_at -- Test of threadscript::predef::f_at */
//! \cond
BOOST_DATA_TEST_CASE(f_at, (std::vector<test::runner_result>{
    {R"(at())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(at(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(at(null, 2, 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(at(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(at(vector(), null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(at(false, 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(at(1, 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(at(-1, 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(at("abc", 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(at(vector(), "key"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(at(hash(), 1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(at(hash(), -2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(at(vector(), 0, "val"))", "val", ""},
    {R"(at(vector(), +0, "val"))", "val", ""},
    {R"(at(hash(), "a", "val"))", "val", ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, false),
            at(v(), 2, "str_val"),
            at(v(), 5, null),
            at(v(), 3, 123),
            at(v(), 4, -45),
            print(at(v(), 0), ",", at(v(), 1), ",",
                at(v(), 2), ",", at(v(), 3), ",",
                at(v(), 4), ",", at(v(), 5)),
            at(v(), 4, "NewValue")
        )
    )", "NewValue", "false,null,str_val,123,-45,null"},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "A", false),
            at(h(), "B", "str_val"),
            at(h(), "C", null),
            at(h(), "D", 123),
            at(h(), "E", -45),
            print(at(h(), "A"), ",", at(h(), "B"), ",",
                at(h(), "C"), ",", at(h(), "D"), ",",
                at(h(), "E")),
            at(h(), "C", "NewValue")
        )
    )", "NewValue", "false,str_val,null,123,-45"},
    {R"(at(vector(), 0))", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Value out of range"
    }, ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, -1),
            at(v(), 1, -2),
            at(v(), 2)
        )
    )", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 6, 13),
        "Runtime error: Value out of range"
    }, ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), +0, -1),
            at(v(), +1, -2),
            at(v(), +2)
        )
    )", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 6, 13),
        "Runtime error: Value out of range"
    }, ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), +0, -1),
            at(v(), +1, -2),
            at(v(), -1)
        )
    )", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 6, 13),
        "Runtime error: Value out of range"
    }, ""},
    {R"(at(hash(), "Key"))", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Value out of range"
    }, ""},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "A", -1),
            at(h(), "B", -2),
            at(h(), "C")
        )
    )", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 6, 13),
        "Runtime error: Value out of range"
    }, ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, -1),
            mt_safe(v()),
            at(v(), 1, -2)
        )
    )", test::exc{
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 6, 13),
        "Runtime error: Read-only value"
    }, ""},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "key1", -1),
            mt_safe(h()),
            at(h(), "key2", -2)
        )
    )", test::exc{
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 6, 13),
        "Runtime error: Read-only value"
    }, ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, clone(-1)),
            mt_safe(v())
        )
    )", test::exc{
        typeid(ts::exception::value_mt_unsafe),
        ts::frame_location("", "", 5, 13),
        "Runtime error: Thread-unsafe value"
    }, ""},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "key", clone(-1)),
            mt_safe(h())
        )
    )", test::exc{
        typeid(ts::exception::value_mt_unsafe),
        ts::frame_location("", "", 5, 13),
        "Runtime error: Thread-unsafe value"
    }, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_bool -- Test of threadscript::predef::f_bool */
//! \cond
BOOST_DATA_TEST_CASE(f_bool, (std::vector<test::runner_result>{
    {R"(bool())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(bool(false, false, null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(bool(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(bool(false))", false, ""},
    {R"(bool(true))", true, ""},
    {R"(bool(0))", true, ""},
    {R"(bool(1))", true, ""},
    {R"(bool(+0))", true, ""},
    {R"(bool(+1))", true, ""},
    {R"(bool(-1))", true, ""},
    {R"(bool(""))", true, ""},
    {R"(bool("abc"))", true, ""},
    {R"(bool(false, false))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(bool(null, true))", true, ""}, // target is null
    {R"(bool(1, true))", true, ""}, // target is constant, but wrong type
    {R"(bool(bool(false), true))", true, ""}, // target is modifiable
    {R"(bool(bool(true), false))", false, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_clone -- Test of threadscript::predef::f_clone */
//! \cond
BOOST_DATA_TEST_CASE(f_clone, (std::vector<test::runner_result>{
    {R"(clone())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(clone(1, 2))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(clone(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(clone(false))", false, ""},
    {R"(clone(true))", true, ""},
    {R"(clone(0))", test::uint_t(0), ""},
    {R"(clone(1))", test::uint_t(1), ""},
    {R"(clone(+2))", test::int_t(2), ""},
    {R"(clone(-3))", test::int_t(-3), ""},
    {R"(clone(""))", "", ""},
    {R"(clone("Abc"))", "Abc", ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_contains -- Test of threadscript::predef::f_contains */
//! \cond
BOOST_DATA_TEST_CASE(f_contains, (std::vector<test::runner_result>{
    {R"(contains())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(contains(null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(contains(null, hash(), "key", 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(contains(hash(), null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(contains(null, "key"))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(contains(false, "key"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(1, "key"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(+1, "key"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains("str", "key"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(vector(), "key"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(hash(), true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(hash(), 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(hash(), +2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(hash(), vector()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(hash(), hash()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(contains(hash(), "key"))", false, ""},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "A", 1),
            at(h(), "B", 1),
            print(contains(h(), "A"), ",", contains(h(), "B"), ",",
                contains(h(), "C"))
        )
    )", nullptr, "true,true,false"},
    {R"(contains(true, hash(), "k"))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(contains(null, hash(), "k"))", false, ""}, // target is null
    {R"(contains(1, hash(), "k"))", false, ""}, // target constant, wrong type
    {R"(contains(clone(true), hash(), "k"))", false, ""}, // target modifiable
    {R"(
        seq(
            var("r", clone(false)),
            var("h", hash()),
            at(h(), "key", "VALUE"),
            print(is_same(contains(r(), h(), "key"), var("r"))),
            r()
        )
    )", true, "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_div -- Test of threadscript::predef::f_div, which also applies to
 * threadscript::predef::f_div_base (the part of implementation shared with
 * threadscript::predef::f_mod) */
//! \cond
BOOST_DATA_TEST_CASE(f_div, (std::vector<test::runner_result>{
    {R"(div())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(div(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(div(null, 2, 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(div(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(div(null, null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(div(1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(div(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(div(false, true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(div(1, +2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(div(-1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(div("a", "a"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {test::u_op("div", 0U, 1U), test::uint_t(0U), ""},
    {test::u_op("div", 0U, 5U), test::uint_t(0U), ""},
    {test::u_op("div", 1U, 1U), test::uint_t(1U), ""},
    {test::u_op("div", 1U, 2U), test::uint_t(0U), ""},
    {test::u_op("div", 6U, 3U), test::uint_t(2U), ""},
    {test::u_op("div", 7U, 3U), test::uint_t(2U), ""},
    {test::u_op("div", 8U, 3U), test::uint_t(2U), ""},
    {test::u_op("div", 9U, 3U), test::uint_t(3U), ""},
    {test::u_op("div", test::u_max, 1U), test::u_max, ""},
    {test::u_op("div", test::u_max, 2U), test::uint_t(test::u_half), ""},
    {test::u_op("div", test::u_max, test::u_max), test::uint_t(1U), ""},
    {test::u_op("div", 0U, 0U), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::u_op("div", 1U, 0U), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::u_op("div", test::u_half, 0U), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::i_op("div", 0, 1), test::int_t(0), ""},
    {test::i_op("div", 0, -5), test::int_t(0), ""},
    {test::i_op("div", 1, 1), test::int_t(1), ""},
    {test::i_op("div", 1, -1), test::int_t(-1), ""},
    {test::i_op("div", -1, 1), test::int_t(-1), ""},
    {test::i_op("div", -1, -1), test::int_t(1), ""},
    {test::i_op("div", 1, 2), test::int_t(0), ""},
    {test::i_op("div", 6, 3), test::int_t(2), ""},
    {test::i_op("div", 7, -3), test::int_t(-2), ""},
    {test::i_op("div", -8, 3), test::int_t(-2), ""},
    {test::i_op("div", -9, -3), test::int_t(3), ""},
    {test::i_op("div", 0, 0), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::i_op("div", 1, 0), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::i_op("div", -2, 0), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::i_op("div", test::i_min, -1), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {R"(div(0, 1, 2))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(div(null, +10, +2))", test::int_t(5), ""}, // target is null
    {R"(div(true, 6, 2))", test::uint_t(3), ""}, // target constant, wrong type
    // target is modifiable
    {R"(
        seq(
            var("r", clone(0)),
            print(is_same(div(var("r"), 3, 2), var("r"))),
            var("r")
        )
    )", test::uint_t(1), "true"},
    {R"(
        seq(
            var("r", clone(+0)),
            print(is_same(div(var("r"), -15, +3), var("r"))),
            var("r")
        )
    )", test::int_t(-5), "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_eq -- Test of threadscript::predef::f_eq */
//! \cond
BOOST_DATA_TEST_CASE(f_eq, (std::vector<test::runner_result>{
    {R"(eq())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(eq(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(eq(null, "2", 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(eq(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(eq(null, null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(eq(1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(eq(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(eq(1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(eq(-1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(eq("1", 1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(eq("1", -1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(eq(false, false))", true, ""},
    {R"(eq(false, true))", false, ""},
    {R"(eq(true, false))", false, ""},
    {R"(eq(true, true))", true, ""},
    {R"(eq(false, 0))", false, ""},
    {R"(eq(true, 0))", true, ""},
    {R"(eq(false, -1))", false, ""},
    {R"(eq(true, -1))", true, ""},
    {R"(eq(false, "str"))", false, ""},
    {R"(eq(true, "str"))", true, ""},
    {R"(eq(0, false))", false, ""},
    {R"(eq(0, true))", true, ""},
    {R"(eq(-1, false))", false, ""},
    {R"(eq(-1, true))", true, ""},
    {R"(eq("str", false))", false, ""},
    {R"(eq("str", true))", true, ""},
    {R"(eq(0, 1))", false, ""},
    {R"(eq(0, 0))", true, ""},
    {R"(eq(12, 23))", false, ""},
    {R"(eq(12, 12))", true, ""},
    {R"(eq(-123, +123))", false, ""},
    {R"(eq(-123, -123))", true, ""},
    {R"(eq(+123, -123))", false, ""},
    {R"(eq(+12, +12))", true, ""},
    {R"(eq(+1, +2))", false, ""},
    {R"(eq(-1, -2))", false, ""},
    {R"(eq(-123, 456))", false, ""},
    {R"(eq(-456, 456))", false, ""},
    {R"(eq(456, -123))", false, ""},
    {R"(eq(456, -456))", false, ""},
    {R"(eq(+123, 456))", false, ""},
    {R"(eq(+456, 456))", true, ""},
    {R"(eq(456, +123))", false, ""},
    {R"(eq(456, +456))", true, ""},
    {R"(eq("", ""))", true, ""},
    {R"(eq("", "xy"))", false, ""},
    {R"(eq("xy", ""))", false, ""},
    {R"(eq("xy", "xy"))", true, ""},
    {R"(eq(false, 1, 2))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(eq(null, 1, 2))", false, ""}, // target is null
    {R"(eq(1, 1, 1))", true, ""}, // target is constant, but wrong type
    {R"(eq(clone(false), 24, 24))", true, ""}, // target is modifiable
    {R"(
        seq(
            var("r", clone(true)),
            print(is_same(eq(var("r"), -1, +2), var("r"))),
            var("r")
        )
    )", false, "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_erase -- Test of threadscript::predef::f_erase */
//! \cond
BOOST_DATA_TEST_CASE(f_erase, (std::vector<test::runner_result>{
    {R"(erase())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(erase(hash(), "key", 3))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(erase(vector(), null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(erase(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(erase(null, 0))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(erase(false))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(-1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase("str"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(false, 1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(1, 1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(-1, 1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase("str", 1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(vector(), false))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(vector(), "str"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(vector(), vector()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(vector(), hash()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(hash(), false))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(hash(), 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(hash(), -2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(hash(), vector()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(hash(), hash()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(erase(vector()))", nullptr, ""},
    {R"(erase(vector(), 0))", nullptr, ""},
    {R"(erase(vector(), 1))", nullptr, ""},
    {R"(erase(hash()))", nullptr, ""},
    {R"(erase(hash(), ""))", nullptr, ""},
    {R"(erase(hash(), "KEY"))", nullptr, ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v(), +0),
            print(size(v()))
        )
    )", nullptr, "0"},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v(), -1),
            print(size(v()))
        )
    )", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 5, 13),
        "Runtime error: Value out of range"
    }, ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            mt_safe(v()),
            erase(v(), 1),
            print(size(v()))
        )
    )", test::exc{
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 6, 13),
        "Runtime error: Read-only value"
    }, ""},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v()),
            print(size(v()))
        )
    )", nullptr, "0"},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v(), 0),
            print(size(v()))
        )
    )", nullptr, "0"},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v(), 1),
            print(size(v()), at(v(), 0))
        )
    )", nullptr, "1a"},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v(), 2),
            print(size(v()), at(v(), 0), at(v(), 1))
        )
    )", nullptr, "2ab"},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v(), 3),
            print(size(v()), at(v(), 0), at(v(), 1), at(v(), 2))
        )
    )", nullptr, "3abc"},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v(), 4),
            print(size(v()), at(v(), 0), at(v(), 1), at(v(), 2))
        )
    )", nullptr, "3abc"},
    {R"(
        seq(
            var("v", vector()),
            at(v(), 0, "a"), at(v(), 1, "b"), at(v(), 2, "c"),
            erase(v(), 10),
            print(size(v()), at(v(), 0), at(v(), 1), at(v(), 2))
        )
    )", nullptr, "3abc"},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "a", "A"), at(h(), "b", "B"),
            erase(h()),
            print(size(h()))
        )
    )", nullptr, "0"},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "a", "A"), at(h(), "b", "B"),
            erase(h(), "x"),
            print(size(h()), at(h(), "a"), at(h(), "b"))
        )
    )", nullptr, "2AB"},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "a", "A"), at(h(), "b", "B"),
            erase(h(), "a"),
            print(size(h()), at(h(), "b"))
        )
    )", nullptr, "1B"},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "a", "A"), at(h(), "b", "B"),
            erase(h(), "a"), erase(h(), "b"),
            print(size(h()))
        )
    )", nullptr, "0"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_fun -- Test of threadscript::predef::f_fun */
//! \cond
BOOST_DATA_TEST_CASE(f_fun, (std::vector<test::runner_result>{
    {R"(fun())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(fun("zero", 0, 1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(fun(null, 0))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(fun(false, 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(fun(1, 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(fun(-2, 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(fun(vector(), 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(fun(hash(), 0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(
        seq(
            fun("zero", 0),
            fun("one", 1),
            print(zero(), ",", zero(1), ",", zero("a", "b")),
            print(" ", one(), ",", one(1), ",", one("a", "b"))
        )
    )", nullptr, "0,0,0 1,1,1"},
    {R"(
        seq(
            fun("sqr", mul(at(_args(), 0), at(_args(), 0))),
            print(sqr(0), sqr(1), sqr(2), sqr(3))
        )
    )", nullptr, "0149"},
    {R"(
        seq(
            fun("narg", size(_args())),
            print(narg(), ","),
            print(narg("a"), ","),
            print(narg("a", "b"), ","),
            print(narg("a", "b", "c"))
        )
    )", nullptr, "0,1,2,3"},
    {R"(
        seq(
            fun("select", at(_args(), at(_args(), 0))),
            print(select(1, "a", "bc", "def"), ","),
            print(select(2, "a", "bc", "def"), ","),
            print(select(3, "a", "bc", "def"))
        )
    )", nullptr, "a,bc,def"},
    {R"(
        seq(
            fun("local_var", seq(
                print("before var: ", v(), "\n"),
                var("v", "function"),
                print("after var: ", v(), "\n")
            )),
            gvar("v", "global"),
            var("v", "script"),
            print("before call: ", v(), "\n"),
            local_var(),
            print("after call: ", v(), "\n")
        )
    )", nullptr,
        "before call: script\n"
        "before var: global\n"
        "after var: function\n"
        "after call: script\n"
    },
    {R"(
        seq(
            fun("local1", "local1:global\n"),
            print(local1()),
            fun("global1", seq(
                print(local1()),
                fun("local1", "local1:global1.local\n"),
                print(local1()),
                fun("local2", "local2\n")
            )),
            fun("global2", print(local2())),
            global1(),
            global2(),
            print(local1())
        )
    )", nullptr,
        "local1:global\n"
        "local1:global\n"
        "local1:global1.local\n"
        "local2\n"
        "local1:global1.local\n"
    },
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_ge -- Test of threadscript::predef::f_ge. Almost all
 * implementation of f_ge is shared with f_lt (except testing the number of
 * arguments), therefore we do only a small number of checks here, assuming
 * that tests of f_lt apply here, too. */
//! \cond
BOOST_DATA_TEST_CASE(f_ge, (std::vector<test::runner_result>{
    {R"(ge())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(ge(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(ge(null, "2", 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(ge(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(ge(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(ge(1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(ge(false, false))", true, ""},
    {R"(ge(false, true))", false, ""},
    {R"(ge(true, false))", true, ""},
    {R"(ge(0, 1))", false, ""},
    {R"(ge(12, 12))", true, ""},
    {R"(ge(123, 12))", true, ""},
    {R"(ge(-123, +123))", false, ""},
    {R"(ge(-123, -123))", true, ""},
    {R"(ge(+123, -123))", true, ""},
    {R"(ge("", "xy"))", false, ""},
    {R"(ge("xy", "xy"))", true, ""},
    {R"(ge("z", "xy"))", true, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_gt -- Test of threadscript::predef::f_gt. Almost all
 * implementation of f_gt is shared with f_lt (except testing the number of
 * arguments), therefore we do only a small number of checks here, assuming
 * that tests of f_lt apply here, too. */
//! \cond
BOOST_DATA_TEST_CASE(f_gt, (std::vector<test::runner_result>{
    {R"(gt())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(gt(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(gt(null, "2", 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(gt(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(gt(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(gt(1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(gt(false, false))", false, ""},
    {R"(gt(false, true))", false, ""},
    {R"(gt(true, false))", true, ""},
    {R"(gt(0, 1))", false, ""},
    {R"(gt(12, 12))", false, ""},
    {R"(gt(123, 12))", true, ""},
    {R"(gt(-123, +123))", false, ""},
    {R"(gt(-123, -123))", false, ""},
    {R"(gt(+123, -123))", true, ""},
    {R"(gt("", "xy"))", false, ""},
    {R"(gt("xy", "xy"))", false, ""},
    {R"(gt("z", "xy"))", true, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_gvar -- Test of threadscript::predef::f_gvar */
//! \cond
BOOST_DATA_TEST_CASE(f_gvar, (std::vector<test::runner_result>{
    {R"(gvar())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(gvar("v"))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(gvar("v", 1, 2))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(gvar(null, 1))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(gvar(false, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(gvar(1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(gvar(-1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(gvar(vector(), 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(gvar(hash(), 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(gvar("v", 123))", nullptr, ""},
    {R"(
        seq(
            gvar("v", 123),
            v()
        )
    )", test::uint_t(123U), ""},
    {R"(
        seq(
            gvar("v", "GLOBAL"),
            var("v", "LOCAL"),
            v()
        )
    )", "LOCAL", ""},
    {R"(
        seq(
            var("v", "LOCAL"),
            gvar("v", "GLOBAL"),
            v()
        )
    )", "LOCAL", ""},
    {R"(
        seq(
            gvar("a", 1),
            gvar("b", 11),
            fun("f", seq(
                var("a", 2),
                gvar("b", 12),
                print("f: ", a(), " ", b(), "\n")
            )),
            fun("g", seq(
                var("a", 3),
                gvar("b", 13),
                print("g: ", a(), " ", b(), "\n")
            )),
            f(),
            print("after f: ", a(), " ", b(), "\n"),
            g(),
            print("after g: ", a(), " ", b(), "\n")
        )
    )", nullptr,
        "f: 2 12\n"
        "after f: 1 12\n"
        "g: 3 13\n"
        "after g: 1 13\n"
    },
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_hash -- Test of threadscript::predef::f_hash */
//! \cond
BOOST_DATA_TEST_CASE(f_hash, (std::vector<test::runner_result>{
    {R"(hash(null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(type(hash()))", "hash", {}},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_if -- Test of threadscript::predef::f_if */
//! \cond
BOOST_DATA_TEST_CASE(f_if, (std::vector<test::runner_result>{
    {R"(if())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(if(null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(if(true))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(if(null, 1, 2, 3))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(if(true, 1, 2, 3))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(if(null, 1))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(if(null, 1, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(if(true, 1))", test::uint_t(1), ""},
    {R"(if(true, 1, 2))", test::uint_t(1), ""},
    {R"(if(false, 1))", nullptr, ""},
    {R"(if(false, 1, 2))", test::uint_t(2), ""},
    {R"(if(0, 1, 2))", test::uint_t(1), ""},
    {R"(if(1, 1, 2))", test::uint_t(1), ""},
    {R"(if(+0, 1, 2))", test::uint_t(1), ""},
    {R"(if(-1, 1, 2))", test::uint_t(1), ""},
    {R"(if("str", 1, 2))", test::uint_t(1), ""},
    {R"(if(true, seq(print("then"), 1)))", test::uint_t(1), "then"},
    {R"(if(true, seq(print("then"), 1), print("else")))",
        test::uint_t(1), "then"},
    {R"(if(false, seq(print("then"), 1)))", nullptr, ""},
    {R"(if(false, print("then"), seq(print("else"), 2)))",
        test::uint_t(2), "else"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_int -- Test of threadscript::predef::f_int */
//! \cond
BOOST_DATA_TEST_CASE(f_int, (std::vector<test::runner_result>{
    {R"(int())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(int(null, 0, null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(int(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(int(null, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(int(true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {test::i_op("int", 0), test::int_t(0), ""},
    {test::i_op("int", 123), test::int_t(123), ""},
    {test::i_op("int", -45), test::int_t(-45), ""},
    {test::i_op("int", test::i_min), test::int_t(test::i_min), ""},
    {test::i_op("int", test::i_n_half), test::int_t(test::i_n_half), ""},
    {test::i_op("int", test::i_p_half), test::int_t(test::i_p_half), ""},
    {test::i_op("int", test::i_max), test::int_t(test::i_max), ""},
    {test::u_op("int", 0U), test::int_t(0), ""},
    {test::u_op("int", 123U), test::int_t(123), ""},
    {test::u_op("int", test::u_half), test::int_t(test::i_max), ""},
    {test::u_op("int", test::u_half + 1U), test::int_t(test::i_min), ""},
    {test::u_op("int", test::u_max - 1U), test::int_t(-2), ""},
    {test::u_op("int", test::u_max), test::int_t(-1), ""},
    {R"(int(""))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(int("+"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(int("-"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(int(" 123"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(int("123 "))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(int("*123"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(int("123a"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(int("0"))", test::int_t(0), ""},
    {R"(int("1234"))", test::int_t(1234), ""},
    {R"(int("+234"))", test::int_t(234), ""},
    {R"(int("-456"))", test::int_t(-456), ""},
    {R"(int("9223372036854775808"))", test::exc{ // 2^63
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Value out of range"
    }, ""},
    {R"(int("18446744073709551616"))", test::exc{ // 2^64
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Value out of range"
    }, ""},
    {R"(int("18446744073709551616 "))", test::exc{ // 2^64, invalid last char
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    // overflow and invalid last char
    {R"(int("184467440737095516161234+"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(int("-9223372036854775809"))", test::exc{ // -(2^63) - 1
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Value out of range"
    }, ""},
    {R"(int(+0, +1))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(int(null, 1))", test::int_t(1), ""}, // target is null
    {R"(int(true, 1))", test::int_t(1), ""}, // target constant, wrong type
    // target is modifiable
    {R"(
        seq(
            var("r", clone(+0)),
            print(is_same(int(var("r"), 2), var("r"))),
            var("r")
        )
    )", test::int_t(2), "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_is_mt_safe -- Test of threadscript::predef::f_is_mt_safe */
//! \cond
BOOST_DATA_TEST_CASE(f_is_mt_safe, (std::vector<test::runner_result>{
    {R"(is_mt_safe())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(is_mt_safe(null, false, null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(is_mt_safe(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(is_mt_safe("constant"))", true, ""},
    {R"(is_mt_safe(clone("writable")))", false, ""},
    {R"(is_mt_safe(false, "const"))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(is_mt_safe(null, "constant"))", true, ""}, // target is null
    {R"(is_mt_safe(1, "constant"))", true, ""}, // target is constant,
                                                // but wrong type
    {R"(is_mt_safe(clone(false), "const"))", true, ""}, // target is modifiable
    {R"(is_mt_safe(clone(true), clone("writable")))", false, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_is_null -- Test of threadscript::predef::f_is_null */
//! \cond
BOOST_DATA_TEST_CASE(f_is_null, (std::vector<test::runner_result>{
    {R"(is_null())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(is_null(false, false, null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(is_null(null))", true, ""},
    {R"(is_null(false))", false, ""},
    {R"(is_null(true))", false, ""},
    {R"(is_null(0))", false, ""},
    {R"(is_null(1))", false, ""},
    {R"(is_null(+0))", false, ""},
    {R"(is_null(+1))", false, ""},
    {R"(is_null(-1))", false, ""},
    {R"(is_null(""))", false, ""},
    {R"(is_null("abc"))", false, ""},
    {R"(is_null(false, false))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(is_null(null, true))", false, ""}, // target is null
    {R"(is_null(1, true))", false, ""}, // target is constant, but wrong type
    {R"(is_null(bool(false), null))", true, ""}, // target is modifiable
    {R"(is_null(bool(true), 1))", false, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_is_same -- Test of threadscript::predef::f_is_same */
//! \cond
BOOST_DATA_TEST_CASE(f_is_same, (std::vector<test::runner_result>{
    {R"(is_same())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(is_same(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(is_same(null, 1, 2, 3))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(is_same(null, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(is_same(1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(is_same(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(is_same(clone(false), 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(is_same(123, 123))", false, ""},
    {R"(seq(var("a", 1), var("b", 1), is_same(var("a"), var("b"))))",
        false, ""},
    {R"(seq(var("a", 1), var("b", var("a")), is_same(var("a"), var("b"))))",
        true, ""},
    {R"(seq(var("a", 1), is_same(var("a"), clone(var("a")))))", false, ""},
    {R"(is_same(false, 1, 2))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(is_same(null, 1, 2))", false, ""}, // target is null
    {R"(is_same(1, 1, 2))", false, ""}, // target is constant, but wrong type
    {R"(is_same(clone(true), 1, 2))", false, ""}, // target is modifiable
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_keys -- Test of threadscript::predef::f_keys */
//! \cond
BOOST_DATA_TEST_CASE(f_keys, (std::vector<test::runner_result>{
    {R"(keys())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(keys(hash(), 1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(keys(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(keys(false))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(keys(0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(keys(+0))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(keys("0"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(keys(vector()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(seq(var("k", keys(hash())), print(type(k())), size(k())))",
        test::uint_t(0U), "vector"},
    {R"(
        seq(
            var("h", hash()),
            at(h(), "Xy", 0), at(h(), "xyz", 1), at(h(), "", 2),
            at(h(), "a", 3), at(h(), "bc", 4),
            var("k", keys(h())),
            print(at(k(), 0), ",", at(k(), 1), ",", at(k(), 2), ",",
                at(k(), 3), ",", at(k(), 4)),
            size(k())
        )
    )", test::uint_t(5U), ",Xy,a,bc,xyz"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_le -- Test of threadscript::predef::f_le. Almost all
 * implementation of f_le is shared with f_lt (except testing the number of
 * arguments), therefore we do only a small number of checks here, assuming
 * that tests of f_lt apply here, too. */
//! \cond
BOOST_DATA_TEST_CASE(f_le, (std::vector<test::runner_result>{
    {R"(le())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(le(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(le(null, "2", 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(le(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(le(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(le(1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(le(false, false))", true, ""},
    {R"(le(false, true))", true, ""},
    {R"(le(true, false))", false, ""},
    {R"(le(0, 1))", true, ""},
    {R"(le(12, 12))", true, ""},
    {R"(le(123, 12))", false, ""},
    {R"(le(-123, +123))", true, ""},
    {R"(le(-123, -123))", true, ""},
    {R"(le(+123, -123))", false, ""},
    {R"(le("", "xy"))", true, ""},
    {R"(le("xy", "xy"))", true, ""},
    {R"(le("z", "xy"))", false, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_lt -- Test of threadscript::predef::f_lt */
//! \cond
BOOST_DATA_TEST_CASE(f_lt, (std::vector<test::runner_result>{
    {R"(lt())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(lt(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(lt(null, "2", 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(lt(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(lt(null, null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(lt(1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(lt(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(lt(1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(lt(-1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(lt("1", 1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(lt("1", -1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(lt(false, false))", false, ""},
    {R"(lt(false, true))", true, ""},
    {R"(lt(true, false))", false, ""},
    {R"(lt(true, true))", false, ""},
    {R"(lt(false, 0))", true, ""},
    {R"(lt(true, 0))", false, ""},
    {R"(lt(false, -1))", true, ""},
    {R"(lt(true, -1))", false, ""},
    {R"(lt(false, "str"))", true, ""},
    {R"(lt(true, "str"))", false, ""},
    {R"(lt(0, false))", false, ""},
    {R"(lt(0, true))", false, ""},
    {R"(lt(-1, false))", false, ""},
    {R"(lt(-1, true))", false, ""},
    {R"(lt("str", false))", false, ""},
    {R"(lt("str", true))", false, ""},
    {R"(lt(0, 1))", true, ""},
    {R"(lt(0, 0))", false, ""},
    {R"(lt(12, 23))", true, ""},
    {R"(lt(12, 12))", false, ""},
    {R"(lt(23, 12))", false, ""},
    {R"(lt(-123, +123))", true, ""},
    {R"(lt(-123, -123))", false, ""},
    {R"(lt(+123, -123))", false, ""},
    {R"(lt(+12, +12))", false, ""},
    {R"(lt(+1, +2))", true, ""},
    {R"(lt(-1, -2))", false, ""},
    {R"(lt(-123, 456))", true, ""},
    {R"(lt(-456, 456))", true, ""},
    {R"(lt(456, -123))", false, ""},
    {R"(lt(456, -456))", false, ""},
    {R"(lt(+123, 456))", true, ""},
    {R"(lt(+456, 456))", false, ""},
    {R"(lt(456, +123))", false, ""},
    {R"(lt(456, +456))", false, ""},
    {R"(lt(456, +567))", true, ""},
    {R"(lt("", ""))", false, ""},
    {R"(lt("", "xy"))", true, ""},
    {R"(lt("xy", ""))", false, ""},
    {R"(lt("xy", "xy"))", false, ""},
    {R"(lt("xy", "yza"))", true, ""},
    {R"(lt(false, 1, 2))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(lt(null, 1, 2))", true, ""}, // target is null
    {R"(lt(1, 1, 1))", false, ""}, // target is constant, but wrong type
    {R"(lt(clone(true), 24, 24))", false, ""}, // target is modifiable
    {R"(
        seq(
            var("r", clone(false)),
            print(is_same(lt(var("r"), -1, +2), var("r"))),
            var("r")
        )
    )", true, "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_mod -- Test of threadscript::predef::f_mod. Almost all
 * implementation of f_mod is shared with f_div, therefore we do only a small
 * number of checks here, assuming that tests of f_div apply here, too. */
//! \cond
BOOST_DATA_TEST_CASE(f_mod, (std::vector<test::runner_result>{
    {test::u_op("mod", 0U, 1U), test::uint_t(0U), ""},
    {test::u_op("mod", 0U, 5U), test::uint_t(0U), ""},
    {test::u_op("mod", 1U, 1U), test::uint_t(0U), ""},
    {test::u_op("mod", 1U, 2U), test::uint_t(1U), ""},
    {test::u_op("mod", 6U, 3U), test::uint_t(0U), ""},
    {test::u_op("mod", 7U, 3U), test::uint_t(1U), ""},
    {test::u_op("mod", 8U, 3U), test::uint_t(2U), ""},
    {test::u_op("mod", 9U, 3U), test::uint_t(0U), ""},
    {test::u_op("mod", 0U, 0U), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::u_op("mod", 1U, 0U), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::u_op("mod", test::u_half, 0U), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::i_op("mod", 0, 1), test::int_t(0), ""},
    {test::i_op("mod", 0, -5), test::int_t(0), ""},
    {test::i_op("mod", 1, 1), test::int_t(0), ""},
    {test::i_op("mod", 1, -1), test::int_t(0), ""},
    {test::i_op("mod", -1, 1), test::int_t(0), ""},
    {test::i_op("mod", -1, -1), test::int_t(0), ""},
    {test::i_op("mod", 1, 2), test::int_t(1), ""},
    {test::i_op("mod", 8, 4), test::int_t(0), ""},
    {test::i_op("mod", 9, -4), test::int_t(1), ""},
    {test::i_op("mod", -10, 4), test::int_t(-2), ""},
    {test::i_op("mod", -11, -4), test::int_t(-3), ""},
    {test::i_op("mod", 0, 0), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::i_op("mod", 1, 0), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::i_op("mod", -2, 0), test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Division by zero"
    }, ""},
    {test::i_op("mod", test::i_min, -1), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_mt_safe -- Test of threadscript::predef::f_mt_safe
 * \todo \c f_mt_safe: test throwing threadscript::exception::value_mt_unsafe */
//! \cond
BOOST_DATA_TEST_CASE(f_mt_safe, (std::vector<test::runner_result>{
    {R"(mt_safe())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(mt_safe(null, false))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(mt_safe(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(mt_safe(1234))", test::uint_t(1234), ""},
    {R"(mt_safe(clone(1234)))", test::uint_t(1234), ""},
    {R"(
        seq(
            var("a", clone("XYZ")),
            print(is_mt_safe(var("a"))),
            print(" ", is_same(mt_safe(var("a")), var("a"))),
            print(" ", var("a")),
            print(" ", is_mt_safe(var("a")))
        )
    )", nullptr, "false true XYZ true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_mul -- Test of threadscript::predef::f_mul */
//! \cond
BOOST_DATA_TEST_CASE(f_mul, (std::vector<test::runner_result>{
    {R"(mul())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(mul(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(mul(null, 2, 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(mul(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(mul(null, null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(mul(1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(mul(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(mul(false, true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(mul(1, +2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(mul(-1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(mul("a", "bc"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {test::u_op("mul", 0U, 0U), test::uint_t(0U), ""},
    {test::u_op("mul", 5U, 12U), test::uint_t(60U), ""},
    {test::u_op("mul", test::u_half, 2U), test::uint_t(test::u_max - 1U), ""},
    {test::u_op("mul", test::u_half + 1U, 2U), test::uint_t(0U), ""},
    {test::u_op("mul", 2U, test::u_half + 2U), test::uint_t(2U), ""},
    {test::i_op("mul", +5, +12), test::int_t(60), ""},
    {test::i_op("mul", -5, +12), test::int_t(-60), ""},
    {test::i_op("mul", +5, -12), test::int_t(-60), ""},
    {test::i_op("mul", -5, -12), test::int_t(60), ""},
    {test::i_op("mul", test::i_p_half, 2), test::int_t(test::i_max - 1), ""},
    {test::i_op("mul", test::i_p_half + 1, 2), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("mul", test::i_p_half, -2), test::int_t(test::i_min + 2), ""},
    {test::i_op("mul", test::i_p_half + 1, -2), test::int_t(test::i_min), ""},
    {test::i_op("mul", test::i_p_half + 2, -2), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("mul", test::i_n_half, 2), test::int_t(test::i_min), ""},
    {test::i_op("mul", test::i_n_half - 1, 2), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("mul", test::i_n_half + 1, -2),
        test::int_t(test::i_max - 1), ""},
    {test::i_op("mul", test::i_n_half, -2), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("mul", -1, test::i_max), test::int_t(test::i_min + 1), ""},
    {test::i_op("mul", -1, test::i_min), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {R"(mul("ijk", 0))", "", ""},
    {R"(mul("ijk", 1))", "ijk", ""},
    {R"(mul("ijk", 2))", "ijkijk", ""},
    {R"(mul(0, "ijk"))", "", ""},
    {R"(mul(1, "ijk"))", "ijk", ""},
    {R"(mul(5, "ijk "))", "ijk ijk ijk ijk ijk ", ""},
    {R"(mul("ijk", +0))", "", ""},
    {R"(mul("ijk", +1))", "ijk", ""},
    {R"(mul("ijk", +2))", "ijkijk", ""},
    {R"(mul(-3, "ijk"))", test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {R"(mul(+0, "ijk"))", "", ""},
    {R"(mul(+1, "ijk"))", "ijk", ""},
    {R"(mul(+5, "ijk "))", "ijk ijk ijk ijk ijk ", ""},
    {R"(mul(-1, "ijk"))", test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {R"(mul(0, 1, 2))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(mul(null, 1, 2))", test::uint_t(2), ""}, // target is null
    {R"(mul(true, 2, 3))", test::uint_t(6), ""}, // target constant, wrong type
    // target is modifiable
    {R"(
        seq(
            var("r", clone(0)),
            print(is_same(mul(var("r"), 2, 3), var("r"))),
            var("r")
        )
    )", test::uint_t(6), "true"},
    {R"(
        seq(
            var("r", clone(+0)),
            print(is_same(mul(var("r"), -1, +3), var("r"))),
            var("r")
        )
    )", test::int_t(-3), "true"},
    {R"(
        seq(
            var("r", clone("")),
            print(is_same(mul(var("r"), "Hello", 2), var("r"))),
            var("r")
        )
    )", "HelloHello", "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_ne -- Test of threadscript::predef::f_ne. Almost all
 * implementation of f_ne is shared with f_eq (except testing the number of
 * arguments), therefore we do only a small number of checks here, assuming
 * that tests of f_eq apply here, too. */
//! \cond
BOOST_DATA_TEST_CASE(f_ne, (std::vector<test::runner_result>{
    {R"(ne())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(ne(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(ne(null, "2", 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(ne(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(ne(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(ne(1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(ne(false, false))", false, ""},
    {R"(ne(false, true))", true, ""},
    {R"(ne(0, 1))", true, ""},
    {R"(ne(12, 12))", false, ""},
    {R"(ne(-123, +123))", true, ""},
    {R"(ne(-123, -123))", false, ""},
    {R"(ne("", "xy"))", true, ""},
    {R"(ne("xy", "xy"))", false, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_not -- Test of threadscript::predef::f_not */
//! \cond
BOOST_DATA_TEST_CASE(f_not, (std::vector<test::runner_result>{
    {R"(not())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(not(null, false, null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(not(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(not(false))", true, ""},
    {R"(not(true))", false, ""},
    {R"(not(1))", false, ""},
    {R"(not(-2))", false, ""},
    {R"(not("str"))", false, ""},
    {R"(not(false, false))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(not(null, true))", false, ""}, // target is null
    {R"(not(1, true))", false, ""}, // target is constant, but wrong type
    {R"(not(clone(true), true))", false, ""}, // target is modifiable
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_or -- Test of threadscript::predef::f_or, which also applies to
 * threadscript::predef::f_or_base (the part of implementation shared with
 * threadscript::predef::f_or_r) */
//! \cond
BOOST_DATA_TEST_CASE(f_or, (std::vector<test::runner_result>{
    {R"(or(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(or(true, null))", true, ""},
    {R"(or(false, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(or(false, null, false))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(or())", false, ""},
    {R"(or(seq(print(1), false)))", false, "1"},
    {R"(or(seq(print(1), true)))", true, "1"},
    {R"(or(1))", true, ""},
    {R"(or(-1))", true, ""},
    {R"(or("str"))", true, ""},
    {R"(or(seq(print(1), false), seq(print(2), false)))", false, "12"},
    {R"(or(seq(print(1), false), seq(print(2), true)))", true, "12"},
    {R"(or(seq(print(1), true), seq(print(2), false)))", true, "1"},
    {R"(or(seq(print(1), true), seq(print(2), true)))", true, "1"},
    {R"(or(seq(print(1), false), seq(print(2), false), seq(print(3), false)))",
        false, "123"},
    {R"(or(seq(print(1), false), seq(print(2), false), seq(print(3), true)))",
        true, "123"},
    {R"(or(seq(print(1), false), seq(print(2), true), seq(print(3), false)))",
        true, "12"},
    {R"(or(seq(print(1), false), seq(print(2), true), seq(print(3), true)))",
        true, "12"},
    {R"(or(seq(print(1), true), seq(print(2), false), seq(print(3), false)))",
        true, "1"},
    {R"(or(seq(print(1), true), seq(print(2), false), seq(print(3), true)))",
        true, "1"},
    {R"(or(seq(print(1), true), seq(print(2), true), seq(print(3), false)))",
        true, "1"},
    {R"(or(seq(print(1), true), seq(print(2), true), seq(print(3), true)))",
        true, "1"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_or_r -- Test of threadscript::predef::f_or_r. Almost all
 * implementation of f_or_r is shared with f_or, therefore we do only a small
 * number of checks here, assuming that tests of f_or apply here, too. */
//! \cond
BOOST_DATA_TEST_CASE(f_or_r, (std::vector<test::runner_result>{
    {R"(or_r())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(or_r(false, true, false))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(or_r(null, true, false))", true, ""}, // target is null
    {R"(or_r(1, true, false))", true, ""}, // target is constant, but a
                                               // wrong type
    {R"(or_r(clone(true), false, false))", false, ""}, // target is modifiable
    {R"(or_r(clone(false), true, true))", true, ""},
    {R"(or_r(clone(false)))", false, ""},
    {R"(or_r(clone(false), false))", false, ""},
    {R"(or_r(clone(false), true))", true, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_print -- Test of threadscript::predef::f_print */
//! \cond
BOOST_DATA_TEST_CASE(f_print, (std::vector<test::runner_result>{
    { R"(print())", nullptr, "" },
    { R"(print(null))", nullptr, "null" },
    { R"(print(false, " ", true))", nullptr, "false true" },
    { R"(print(+0, " ", +1, " ", -1, " ", +234, " ", -567))", nullptr,
        "0 1 -1 234 -567" },
    { R"(print(0, " ", 1, " ", 234))", nullptr, "0 1 234" },
    { R"(print("ABC"))", nullptr, "ABC" },
    { R"(print("\0\t\n\r\"\\"))", nullptr, "\0\t\n\r\"\\"s },
    { R"(print("\x41\x4a\x5A\X6c\X6B"))", nullptr, "AJZlk" },
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_seq -- Test of threadscript::predef::f_seq */
//! \cond
BOOST_DATA_TEST_CASE(f_seq, (std::vector<test::runner_result>{
    { R"(seq())", nullptr, "" },
    { R"(seq(print(1)))", nullptr, "1" },
    { R"(seq(print(1), print(2)))", nullptr, "12" },
    { R"(seq(print(1), print(2), print(3)))", nullptr, "123" },
    { R"(seq(1))", test::uint_t(1), "" },
    { R"(seq(false, 2))", test::uint_t(2), "" },
    { R"(seq(1, 2, null))", nullptr, "" },
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_size -- Test of threadscript::predef::f_size */
//! \cond
BOOST_DATA_TEST_CASE(f_size, (std::vector<test::runner_result>{
    {R"(size())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(size(null, 1, 2))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(size(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(size(null, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(size(false))", test::uint_t(1U), ""},
    {R"(size(true))", test::uint_t(1U), ""},
    {R"(size(0))", test::uint_t(1U), ""},
    {R"(size(12))", test::uint_t(1U), ""},
    {R"(size(+34))", test::uint_t(1U), ""},
    {R"(size(-56))", test::uint_t(1U), ""},
    {R"(size(""))", test::uint_t(0U), ""},
    {R"(size("X"))", test::uint_t(1U), ""},
    {R"(size("ABCDEF"))", test::uint_t(6U), ""},
    {R"(size(vector()))", test::uint_t(0U), ""},
    {R"(seq(var("v", vector()), at(v(), 3, "str"), size(v())))",
        test::uint_t(4U), ""},
    {R"(size(hash()))", test::uint_t(0U), ""},
    {R"(seq(var("h", hash()), at(h(), "a", -1), at(h(), "b", -2), size(h())))",
        test::uint_t(2U), ""},
    {R"(size(0, 1))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(size(null, 1))", test::uint_t(1), ""}, // target is null
    {R"(size(true, 1))", test::uint_t(1), ""}, // target constant, bad type
    // target is modifiable
    {R"(
        seq(
            var("r", clone(0)),
            print(is_same(size(r(), "abc"), r())),
            r()
        )
    )", test::uint_t(3), "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond


/*! \file
 * \test \c f_sub -- Test of threadscript::predef::f_sub */
//! \cond
BOOST_DATA_TEST_CASE(f_sub, (std::vector<test::runner_result>{
    {R"(sub())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(sub(1))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(sub(null, 2, 3, 4))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(sub(null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(sub(null, null, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(sub(1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(sub(null, 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(sub(false, true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(sub(1, +2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(sub(-1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(sub("a", "a"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {test::u_op("sub", 0, 0), test::uint_t(0), ""},
    {test::u_op("sub", 123, 12), test::uint_t(111), ""},
    {test::u_op("sub", test::u_half, test::u_half),
        test::uint_t(0), ""},
    {test::u_op("sub", test::u_half, test::u_half + 1U),
        test::uint_t(test::u_max), ""},
    {test::u_op("sub", test::u_half, test::u_half + 2U),
        test::uint_t(test::u_max - 1U), ""},
    {test::u_op("sub", test::u_half - 1U, test::u_half + 2U),
        test::uint_t(test::u_max - 2U), ""},
    {test::u_op("sub", 0U, 0U), test::uint_t(0U), ""},
    {test::u_op("sub", 0U, 1U), test::uint_t(test::u_max), ""},
    {test::u_op("sub", 0U, test::u_half),
        test::uint_t(test::u_half + 2U), ""},
    {test::i_op("sub", 0, 0), test::int_t(0), ""},
    {test::i_op("sub", 12, 34), test::int_t(-22), ""},
    {test::i_op("sub", -12, 34), test::int_t(-46), ""},
    {test::i_op("sub", 12, -34), test::int_t(46), ""},
    {test::i_op("sub", -12, -34), test::int_t(22), ""},
    {test::i_op("sub", test::i_p_half, test::i_n_half),
        test::int_t(test::i_max), ""},
    {test::i_op("sub", test::i_p_half, test::i_n_half - 1), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("sub", test::i_p_half + 1, test::i_n_half), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("sub", test::i_n_half, test::i_p_half + 1),
        test::int_t(test::i_min), ""},
    {test::i_op("sub", test::i_n_half, test::i_p_half + 2), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("sub", test::i_n_half - 2, test::i_p_half), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {test::i_op("sub", test::i_max, 0), test::int_t(test::i_max), ""},
    {test::i_op("sub", 0, test::i_max), test::int_t(test::i_min + 1), ""},
    {test::i_op("sub", 0, test::i_min), test::exc{
        typeid(ts::exception::op_overflow),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Overflow"
    }, ""},
    {R"(sub(0, 1, 2))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(sub(null, +1, +2))", test::int_t(-1), ""}, // target is null
    {R"(sub(true, 3, 1))", test::uint_t(2), ""}, // target constant, wrong type
    // target is modifiable
    {R"(
        seq(
            var("r", clone(0)),
            print(is_same(sub(var("r"), 3, 2), var("r"))),
            var("r")
        )
    )", test::uint_t(1), "true"},
    {R"(
        seq(
            var("r", clone(+0)),
            print(is_same(sub(var("r"), -1, +3), var("r"))),
            var("r")
        )
    )", test::int_t(-4), "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_substr -- Test of threadscript::predef::f_substr */
//! \cond
BOOST_DATA_TEST_CASE(f_substr, (std::vector<test::runner_result>{
    {R"(substr())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(substr("abc"))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(substr("abc", 1, 2, 3))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(substr(null, 1))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(substr(null, 1, 2))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(substr("abc", null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(substr("abc", null, 1))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(substr("abc", 1, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(substr(false, 1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr(0, 1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr(+0, 1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr(vector(), 1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr(hash(), 1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("abcd", true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("abcd", "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("abcd", vector()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("abcd", hash()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("abcd", 1, true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("abcd", 1, "1"))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("abcd", 1, vector()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("abcd", 1, hash()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(substr("XYZxyz", -1, 2))", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Value out of range"
    }, ""},
    {R"(substr("XYZxyz", 1, -2))", test::exc{
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Value out of range"
    }, ""},
    {R"(substr("", 0))", "", ""},
    {R"(substr("", 1))", "", ""},
    {R"(substr("", 10))", "", ""},
    {R"(substr("", 0, 0))", "", ""},
    {R"(substr("", 0, 1))", "", ""},
    {R"(substr("", 0, 5))", "", ""},
    {R"(substr("X", 0))", "X", ""},
    {R"(substr("X", 1))", "", ""},
    {R"(substr("X", 2))", "", ""},
    {R"(substr("X", 0, 0))", "", ""},
    {R"(substr("X", 0, 1))", "X", ""},
    {R"(substr("X", 0, 2))", "X", ""},
    {R"(substr("X", 1, 0))", "", ""},
    {R"(substr("X", 1, 1))", "", ""},
    {R"(substr("abc", 0))", "abc", ""},
    {R"(substr("abc", 1))", "bc", ""},
    {R"(substr("abc", +1))", "bc", ""},
    {R"(substr("abc", 2))", "c", ""},
    {R"(substr("abc", 3))", "", ""},
    {R"(substr("abc", 4))", "", ""},
    {R"(substr("abc", 0, 0))", "", ""},
    {R"(substr("abc", 0, 1))", "a", ""},
    {R"(substr("abc", 0, +1))", "a", ""},
    {R"(substr("abc", 0, 2))", "ab", ""},
    {R"(substr("abc", 0, 3))", "abc", ""},
    {R"(substr("abc", 0, 4))", "abc", ""},
    {R"(substr("abc", 0, 5))", "abc", ""},
    {R"(substr("abc", 1, 0))", "", ""},
    {R"(substr("abc", 1, 1))", "b", ""},
    {R"(substr("abc", 1, 2))", "bc", ""},
    {R"(substr("abc", 1, +2))", "bc", ""},
    {R"(substr("abc", 1, 3))", "bc", ""},
    {R"(substr("abc", +1, +3))", "bc", ""},
    {R"(substr("abc", 1, 4))", "bc", ""},
    {R"(substr("abc", 2, 0))", "", ""},
    {R"(substr("abc", 2, 1))", "c", ""},
    {R"(substr("abc", 2, 2))", "c", ""},
    {R"(substr("abc", 2, 3))", "c", ""},
    {R"(substr("abc", 3, 0))", "", ""},
    {R"(substr("abc", 3, 1))", "", ""},
    {R"(substr("abc", 3, 3))", "", ""},
    {R"(substr("abc", 4, 0))", "", ""},
    {R"(substr("abc", 4, 1))", "", ""},
    {R"(substr("abc", 4, 2))", "", ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_throw -- Test of threadscript::predef::f_throw */
//! \cond
BOOST_DATA_TEST_CASE(f_throw, (std::vector<test::runner_result>{
    {R"(throw("a", "b"))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(throw(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(throw(true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(throw(1))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(throw(-2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(throw(vector()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(throw(hash()))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(throw())", test::exc{
        typeid(ts::exception::op_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad operation"
    }, ""},
    {R"(
        try(
            throw("Error message"),
            "", seq(
                print("handler"),
                throw()
            )
        )
    )", test::exc{
        typeid(ts::exception::script_throw),
        ts::frame_location("", "", 3, 13),
        "Error message"
    }, "handler"},
    {R"(throw("Error message"))", test::exc{
        typeid(ts::exception::script_throw),
        ts::frame_location("", "", 1, 1),
        "Error message"
    }, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_try -- Test of threadscript::predef::f_try */
//! \cond
BOOST_DATA_TEST_CASE(f_try, (std::vector<test::runner_result>{
    {R"(try())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(try(null, ""))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(try(null, "", null, "!"))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(try(null))", nullptr, ""},
    {R"(try(false))", false, ""},
    {R"(try(123))", test::uint_t(123), ""},
    {R"(try(-345))", test::int_t(-345), ""},
    {R"(try("ok"))", "ok", ""},
    {R"(try(add(1, 2)))", test::uint_t(3), ""},
    {R"(try(throw(""), "", null))", nullptr, ""},
    {R"(try(throw(""), null, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(try(throw(""), "NOMATCH", null, "", null))", nullptr, ""},
    {R"(try(throw(""), "NOMATCH", null, null, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(try(throw(""), false, null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), "NOMATCH", null, false, null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), 0, null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), "NOMATCH", null, 0, null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), -10, null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), "NOMATCH", null, -10, null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), vector(), null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), "NOMATCH", null, vector(), null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), hash(), null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(try(throw(""), "NOMATCH", null, hash(), null))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(
        try(
            div(1, 0),
            "", "Exception"
        )
    )", "Exception", ""},
    {R"(
        try(
            div(1, 0),
            "op_div_zero", "Exception"
        )
    )", "Exception", ""},
    {R"(
        try(
            div(1, 0),
            "base", "Exception" # Base classes not matching
        )
    )", test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 3, 13),
        "Runtime error: Division by zero"
    }, ""},
    {R"(
        try(
            div(1, 0),
            "operation", "Exception" # Base classes not matching
        )
    )", test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 3, 13),
        "Runtime error: Division by zero"
    }, ""},
    {R"(
        try(
            div(1, 0),
            "!op_div_zero", "Exception" # This is matching of msg, not type
        )
    )", test::exc{
        typeid(ts::exception::op_div_zero),
        ts::frame_location("", "", 3, 13),
        "Runtime error: Division by zero"
    }, ""},
    {R"(
        try(
            throw("Exception"),
            "script_throw", "Handled"
        )
    )", "Handled", ""},
    {R"(
        try(
            throw("Exception"),
            "!Exception", "Handled"
        )
    )", "Handled", ""},
    {R"(
        try(
            throw("Exception"),
            "!script_throw", "Handled" # This is matching of msg, not type
        )
    )", test::exc{
        typeid(ts::exception::script_throw),
        ts::frame_location("", "", 3, 13),
        "Exception"
    }, ""},
    {R"(
        try(
            throw("Exception"),
            "Exception", "Handled" # This is matching of type, not msg
        )
    )", test::exc{
        typeid(ts::exception::script_throw),
        ts::frame_location("", "", 3, 13),
        "Exception"
    }, ""},
    {R"(
        try(
            throw("Exception"),
            "!Exception", "Matched msg",
            "op_div_zero", "Matched type",
            "", "Default"
        )
    )", "Matched msg", ""},
    {R"(
        try(
            div(1, 0),
            "!Exception", "Matched msg",
            "op_div_zero", "Matched type",
            "", "Default"
        )
    )", "Matched type", ""},
    {R"(
        try(
            clone(),
            "!Exception", "Matched msg",
            "op_div_zero", "Matched type",
            "", "Default"
        )
    )", "Default", ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_type -- Test of threadscript::predef::f_type */
//! \cond
BOOST_DATA_TEST_CASE(f_type, (std::vector<test::runner_result>{
    {R"(type())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(type(false, false, null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(type(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(type(false))", "bool", ""},
    {R"(type(true))", "bool", ""},
    {R"(type(0))", "unsigned", ""},
    {R"(type(1))", "unsigned", ""},
    {R"(type(+0))", "int", ""},
    {R"(type(+1))", "int", ""},
    {R"(type(-1))", "int", ""},
    {R"(type(""))", "string", ""},
    {R"(type("abc"))", "string", ""},
    {R"(type("abc", -1))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(type(null, -1))", "int", ""}, // target is null
    {R"(type(1, -1))", "int", ""}, // target is constant, but wrong type
    {R"(type(type(false), 123))", "unsigned", ""}, // target is modifiable
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_unsigned -- Test of threadscript::predef::f_unsigned */
//! \cond
BOOST_DATA_TEST_CASE(f_unsigned, (std::vector<test::runner_result>{
    {R"(unsigned())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(unsigned(null, 0, null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(unsigned(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(unsigned(null, null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(unsigned(true))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {test::i_op("unsigned", 0), test::uint_t(0), ""},
    {test::i_op("unsigned", 123), test::uint_t(123), ""},
    {test::i_op("unsigned", -45), test::uint_t(test::u_max - 44U), ""},
    {test::i_op("unsigned", test::i_min), test::uint_t(test::u_half + 1U), ""},
    {test::i_op("unsigned", test::i_n_half),
        test::uint_t(test::u_max - test::i_p_half), ""},
    {test::i_op("unsigned", test::i_p_half), test::uint_t(test::i_p_half), ""},
    {test::i_op("unsigned", test::i_max), test::uint_t(test::i_max), ""},
    {test::u_op("unsigned", 0U), test::uint_t(0), ""},
    {test::u_op("unsigned", 123U), test::uint_t(123), ""},
    {test::u_op("unsigned", test::u_half), test::u_half, ""},
    {test::u_op("unsigned", test::u_half + 1U),
        test::uint_t(test::u_half + 1U), ""},
    {test::u_op("unsigned", test::u_max - 1U),
        test::uint_t(test::u_max - 1U), ""},
    {test::u_op("unsigned", test::u_max), test::u_max, ""},
    {R"(unsigned(""))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned("+"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned("-"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned(" 123"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned("123 "))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned("*123"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned("123a"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned("0"))", test::uint_t(0), ""},
    {R"(unsigned("1234"))", test::uint_t(1234), ""},
    {R"(unsigned("+234"))", test::uint_t(234), ""},
    {R"(unsigned("-456"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned("18446744073709551616"))", test::exc{ // 2^64
        typeid(ts::exception::value_out_of_range),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Value out of range"
    }, ""},
    {R"(unsigned("18446744073709551616 "))", test::exc{ // 2^64, bad last char
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    // overflow and invalid last char
    {R"(unsigned("184467440737095516161234+"))", test::exc{
        typeid(ts::exception::value_bad),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value"
    }, ""},
    {R"(unsigned(0, 1))", test::exc{ // target is constant literal
        typeid(ts::exception::value_read_only),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Read-only value"
    }, ""},
    {R"(unsigned(null, 1))", test::uint_t(1), ""}, // target is null
    {R"(unsigned(true, 1))", test::uint_t(1), ""}, // target constant, bad type
    // target is modifiable
    {R"(
        seq(
            var("r", clone(0)),
            print(is_same(unsigned(var("r"), 2), var("r"))),
            var("r")
        )
    )", test::uint_t(2), "true"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_var -- Test of threadscript::predef::f_var */
//! \cond
BOOST_DATA_TEST_CASE(f_var, (std::vector<test::runner_result>{
    {R"(var())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(var("v", 1, 2))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(var(null))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(var(null, 1))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(var(1, 2))", test::exc{
        typeid(ts::exception::value_type),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad value type"
    }, ""},
    {R"(var("foo_goo"))", test::exc{
        typeid(ts::exception::unknown_symbol),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Symbol not found: foo_goo"
    }, ""},
    {R"(var("v", 123))", test::uint_t(123), ""},
    {R"(seq(var("v", 123), var("v")))", test::uint_t(123), ""},
    {R"(var("str", "\0\t\n\r\"\\"))", "\0\t\n\r\"\\"sv, ""},
    {R"(var("str", "\x41\x4a\x5A\X6c\X6B"))", "AJZlk", ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_vector -- Test of threadscript::predef::f_vector */
//! \cond
BOOST_DATA_TEST_CASE(f_vector, (std::vector<test::runner_result>{
    {R"(vector(null))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(type(vector()))", "vector", {}},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_while -- Test of threadscript::predef::f_while */
//! \cond
BOOST_DATA_TEST_CASE(f_while, (std::vector<test::runner_result>{
    {R"(while())", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(while(true))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(while(true, 1, 2))", test::exc{
        typeid(ts::exception::op_narg),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Bad number of arguments"
    }, ""},
    {R"(while(null, 1))", test::exc{
        typeid(ts::exception::value_null),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Null value"
    }, ""},
    {R"(while(false, 1))", nullptr, ""},
    {R"(
        seq(
            var("c0", "c1"),
            var("c1", "c2"),
            var("c2", "c3"),
            var("c3", false),
            var("cond", "c0"),
            while(
                var("cond"),
                seq(
                    print(var("cond"), "\n"),
                    var("cond", var(var("cond")))
                )
            )
        )
    )", false, "c0\nc1\nc2\nc3\n"},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c unknown -- Tests an exception throws in an unknown command or
 * function is called (with or without arguments) */
//! \cond
BOOST_DATA_TEST_CASE(unknown, (std::vector<test::runner_result>{
    {R"(nonexistent())", test::exc{
        typeid(ts::exception::unknown_symbol),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Symbol not found: nonexistent"
    }, ""},
    {R"(nonexistent(1, 2, 3))", test::exc{
        typeid(ts::exception::unknown_symbol),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Symbol not found: nonexistent"
    }, ""},
    {R"(seq(1, nonexistent(2), 3))", test::exc{
        typeid(ts::exception::unknown_symbol),
        ts::frame_location("", "", 1, 8),
        "Runtime error: Symbol not found: nonexistent"
    }, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c variable -- Tests getting the value of a non-function variable by
 * calling it as a function */
//! \cond
BOOST_DATA_TEST_CASE(variable, (std::vector<test::runner_result>{
    {R"(nonexistent())", test::exc{
        typeid(ts::exception::unknown_symbol),
        ts::frame_location("", "", 1, 1),
        "Runtime error: Symbol not found: nonexistent"
    }, ""},
    {R"(seq(var("v", -123), v()))", test::int_t(-123), ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c loop -- Tests using various function to create a for-style loop */
//! \cond
BOOST_DATA_TEST_CASE(loop, (std::vector<test::runner_result>{
    {R"(
        seq(
            var("i", 0),
            while(lt(i(), 10), seq(
                print(i()),
                var("i", add(i(), 1))
            ))
        )
        )", test::uint_t(10U), "0123456789"},
}))
{
    test::check_runner(sample);
}
//! \endcond
