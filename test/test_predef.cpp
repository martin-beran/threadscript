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
    std::string script;
    std::variant<std::nullptr_t, bool, int_t, uint_t, const char*, exc>
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
            } else if (auto ps = std::get_if<const char*>(&sample.result)) {
                check_value<ts::value_string>(result, ps);
            }
        }
    }
    BOOST_CHECK_EQUAL(runner.std_out.view(), sample.std_out);
}

} // namespace test
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
    {R"(is_same(1, 1, 2))", false, ""}, // target is constant, but wrong type
    {R"(is_same(clone(true), 1, 2))", false, ""}, // target is modifiable
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
    {R"(type(1, -1))", "int", ""}, // target is constant, but wrong type
    {R"(type(type(false), 123))", "unsigned", ""}, // target is modifiable
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
}))
{
    test::check_runner(sample);
}
//! \endcond
