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
    {R"(bool(bool(false), true))", true, ""}, // target is modifiable
    {R"(bool(bool(true), false))", false, ""},
}))
{
    test::check_runner(sample);
}
//! \endcond

/*! \file
 * \test \c f_print -- Test of threadscript::predef::f_print */
//! \cond
BOOST_AUTO_TEST_CASE(f_print)
{
    test::script_runner runner(R"(
print(
    null, " ",
    false, " ", true, " ",
    +0, " ", +1, " ", -1, " ", +234, " ", -567, " ",
    0, " ", 1, " ", 234, " ",
    "ABC"
)
    )");
    BOOST_CHECK(runner.run() == nullptr);
    BOOST_CHECK_EQUAL(runner.std_out.view(),
                      "null "sv
                      "false true "
                      "0 1 -1 234 -567 "
                      "0 1 234 "
                      "ABC");
}
//! \endcond

/*! \file
 * \test \c f_seq -- Test of threadscript::predef::f_seq */
//! \cond
BOOST_AUTO_TEST_CASE(f_seq)
{
    test::script_runner runner(R"(
seq(
    print(1),
    print(2),
    print(3)
)
    )");
    BOOST_CHECK(runner.run() == nullptr);
    BOOST_CHECK_EQUAL(runner.std_out.view(), "123"sv);
}
//! \endcond
