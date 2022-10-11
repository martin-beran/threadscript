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
    std::shared_ptr<ts::symbol_table> sh_vars;
    ts::virtual_machine vm{alloc};
    std::string script;
};

script_runner::script_runner(std::string script,
                             std::shared_ptr<ts::symbol_table> sh_vars):
    sh_vars(std::move(sh_vars)), script(std::move(script))
{
    // Redirect standard output to a string stream
    vm.std_out = &std_out;
    // Register default predefined built-in native commands and functions
    if (!this->sh_vars)
        this->sh_vars = ts::predef_symbols(alloc);
    // Set global shared symbols to VM
    vm.sh_vars = this->sh_vars;
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

} // namespace test
//! [parse_run]

//! [make_sh_vars]
namespace test {

// Register native object classes
template <class ...Objects> std::shared_ptr<ts::symbol_table> make_sh_vars()
{
    auto sh_vars = ts::predef_symbols(alloc);
    (..., Objects::register_constructor(*sh_vars, true));
    return sh_vars;
}

} // namespace test
//! [make_sh_vars]

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace test {

struct script_runner_threads: script_runner {
    using script_runner::script_runner;
    ts::value::value_ptr run();
};

ts::value::value_ptr script_runner_threads::run()
{
    // Parse and run the script, expect it to define variable num_threads and
    // functions f_main, f_thread
    auto parsed = ts::parse_code(vm.get_allocator(), script, "string");
    {
        ts::state s_prepare{vm};
        BOOST_REQUIRE_NO_THROW(
            try {
                parsed->eval(s_prepare);
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
        // Store definitions in the global symbol table
        for (auto&& sym: s_prepare.t_vars.csymbols())
            sh_vars->insert(sym.first, sym.second);
    }
    auto name_num_threads = "num_threads";
    auto name_f_main = "f_main";
    auto name_f_thread = "f_thread";
    auto num_threads = dynamic_pointer_cast<ts::value_unsigned>(
                        sh_vars->lookup(name_num_threads).value_or(nullptr));
    BOOST_REQUIRE_NE(num_threads, nullptr);
    auto f_main =
        dynamic_pointer_cast<ts::value_function>(sh_vars->lookup(name_f_main).
                                                 value_or(nullptr));
    BOOST_REQUIRE_NE(f_main, nullptr);
    auto f_thread =
        dynamic_pointer_cast<ts::value_function>(sh_vars->lookup(name_f_thread).
                                                 value_or(nullptr));
    BOOST_REQUIRE_NE(f_thread, nullptr);
    // Run functions in threads
    const size_t nt = num_threads->cvalue();
    std::vector<std::thread> threads;
    for (size_t t = 0; t < nt; ++t)
        threads.emplace_back([this, &name_f_thread, &f_thread, t]() {
            auto args = ts::value_vector::create(vm.get_allocator());
            auto arg_t = ts::value_unsigned::create(vm.get_allocator());
            arg_t->value() = t;
            args->value().push_back(arg_t);
            ts::state s_thread{vm};
            try {
                f_thread->call(s_thread, name_f_thread, args);
            } catch (std::exception& e) {
                std::cout << "thread=" << t << " exception=" << e.what() <<
                    std::endl;
            }
        });
    ts::value::value_ptr result = nullptr;
    std::exception_ptr exc{};
    try {
        ts::state s_main{vm};
        result = f_main->call(s_main, name_f_main);
    } catch (...) {
        exc = std::current_exception();
    }
    // Wait for threads and finish
    for (auto&& t: threads)
        t.join();
    if (exc)
        std::rethrow_exception(exc);
    return result;
}

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

template <class R = script_runner>
void check_runner(const runner_result& sample,
                  std::shared_ptr<ts::symbol_table> sh_vars = nullptr)
{
    R runner(sample.script, sh_vars);
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
