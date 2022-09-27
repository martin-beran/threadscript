#pragma once

/*! \file
 * Helpers for running scripts and checking results in tests
 *
 * See, e.g., programs test_predef.cpp and test_shared_vector.cpp for examples
 * how to use this file.
 */
//!\cond
//! [parse_run]
namespace ts = threadscript;

namespace test {

ts::allocator_any alloc;

struct script_runner {
    script_runner(std::string script,
                  std::shared_ptr<ts::symbol_table> sh_vars = nullptr);
    ts::value::value_ptr run();
    std::ostringstream std_out;
    ts::virtual_machine vm{alloc};
    std::string script;
};

script_runner::script_runner(std::string script,
                             std::shared_ptr<ts::symbol_table> sh_vars):
    script(std::move(script))
{
    // Redirect standard output to a string stream
    vm.std_out = &std_out;
    // Register default predefined built-in native commands and functions
    if (!sh_vars)
        sh_vars = ts::predef_symbols(alloc);
    // Set global shared symbols to VM
    vm.sh_vars = sh_vars;
}

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

// Register native object classes
template <class ...Objects> std::shared_ptr<ts::symbol_table> make_sh_vars()
{
    auto sh_vars = ts::predef_symbols(alloc);
    (..., Objects::register_constructor(*sh_vars, true));
    return sh_vars;
}

} // namespace test
//! [parse_run]

using namespace std::string_literals;
using namespace std::string_view_literals;

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

void check_runner(const runner_result& sample,
                  std::shared_ptr<ts::symbol_table> sh_vars = nullptr)
{
    test::script_runner runner(sample.script, sh_vars);
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
            BOOST_CHECK_EQUAL(result, nullptr);
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

} // namespace test
//!\endcond
