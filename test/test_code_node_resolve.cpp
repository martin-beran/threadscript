/*! \file
 * \brief Tests of threadscript::basic_code_node::resolve() and
 * threadscript::basic_script::resolve()
 */

//! \cond
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE code_node_resolve
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

namespace ts = threadscript;

namespace test {

struct resolving_runner {
    resolving_runner(std::string script1, std::string script2, bool resolve,
                     bool replace, bool remove);
    ~resolving_runner();
    std::string run(ts::a_string fun);
    void no_fun(ts::a_string fun);
    std::ostringstream std_out;
    ts::virtual_machine vm{ts::allocator_any()};
    std::string script1;
    std::string script2;
    ts::script::script_ptr parsed1;
    ts::script::script_ptr parsed2;
    ts::symbol_table t_vars;
};

resolving_runner::resolving_runner(std::string script1, std::string script2,
                                   bool resolve, bool replace, bool remove):
    script1(script1), script2(script2), t_vars(vm.get_allocator(), nullptr)
{
    auto sh_vars = ts::predef_symbols(vm.get_allocator());
    vm.sh_vars = sh_vars;
    vm.std_out = &std_out;
    BOOST_REQUIRE_NO_THROW(
        try {
            parsed1 = ts::parse_code(vm.get_allocator(), script1, "string1");
        } catch (std::exception& e) {
            BOOST_TEST_INFO("exception: " << e.what());
            throw;
        });
    BOOST_REQUIRE_NO_THROW(
        try {
            parsed2 = ts::parse_code(vm.get_allocator(), script2, "string2");
        } catch (std::exception& e) {
            BOOST_TEST_INFO("exception: " << e.what());
            throw;
        });
    ts::state thread1{vm};
    BOOST_REQUIRE_NO_THROW(
        try {
            parsed1->eval(thread1);
        } catch (std::exception& e) {
            BOOST_TEST_INFO("exception: " << e.what());
            throw;
        });
    t_vars = std::move(thread1.t_vars);
    if (resolve)
        parsed1->resolve(t_vars, false, false);
    ts::state thread2{vm};
    BOOST_REQUIRE_NO_THROW(
        try {
            parsed2->eval(thread2);
        } catch (std::exception& e) {
            BOOST_TEST_INFO("exception: " << e.what());
            throw;
        });
    if (resolve)
        parsed1->resolve(thread2.t_vars, replace, remove);
}

resolving_runner::~resolving_runner()
{
    if (parsed1)
        parsed1->unresolve();
    if (parsed2)
        parsed2->unresolve();
}

std::string resolving_runner::run(ts::a_string fun)
{
    auto pfun = dynamic_pointer_cast<ts::value_function>(t_vars.lookup(fun).
                                                         value_or(nullptr));
    BOOST_REQUIRE_NE(pfun, nullptr);
    ts::state thread{vm};
    thread.t_vars = t_vars;
    auto args = ts::value_vector::create(vm.get_allocator());
    BOOST_REQUIRE_NO_THROW(
        try {
            pfun->call(thread, fun, args);
        } catch (std::exception& e) {
            BOOST_TEST_INFO("exception: " << e.what());
            throw;
        });
    auto result = std_out.str();
    std_out = {};
    return result;
}

void resolving_runner::no_fun(ts::a_string fun)
{
    auto pfun = dynamic_pointer_cast<ts::value_function>(t_vars.lookup(fun).
                                                         value_or(nullptr));
    BOOST_REQUIRE_EQUAL(pfun, nullptr);
}

std::string script1{R"SCRIPT(
seq(
    fun("resolve", seq(
        print("resolve(", at(_args(), 0), ")")
    )),
    fun("only1", seq(
        print("only1()")
    )),
    fun("check", seq(
        fun("resolve", seq(
            print("resolve_dynamic(", at(_args(), 0), ")")
        )),
        fun("only1", seq(
            print("only1_dynamic()")
        )),
        resolve("1"),
        only1(),
        print(v_unsafe(), v_safe()),
        print(is_null(null))
    )),
    fun("call_only1", seq(
        only1()
    )),
    fun("call_only2", seq(
        only2()
    )),
    gvar("v_unsafe", clone("unsafe1")),
    gvar("v_safe", "safe1")
)
)SCRIPT"};

std::string script2{R"SCRIPT(
seq(
    fun("resolve", seq(
        print("resolve_script2(", at(_args(), 0), ")")
    )),
    fun("only2", seq(
        print("only2()")
    )),
    fun("is_null", "PredefReplaced"),
    gvar("v_unsafe", clone("unsafe2")),
    gvar("v_safe", "safe2")
)
)SCRIPT"};
} // namespace test
//! \endcond

/*! \file
 * \test \c no_resolve -- Checks repeated lookups if names are not resolved */
//! \cond
BOOST_AUTO_TEST_CASE(no_resolve)
{
    test::resolving_runner runner(test::script1, test::script2,
                                  false, false, false);
    BOOST_CHECK_EQUAL(runner.run("check"),
                      "resolve_dynamic(1)only1_dynamic()unsafe1safe1true");
    runner.no_fun("only2");
}
//! \endcond

/*! \file
 * \test \c resolve -- Checks resolved names, with \c replace=false,
 * \c remove=false */
//! \cond
BOOST_AUTO_TEST_CASE(resolve)
{
    test::resolving_runner runner(test::script1, test::script2,
                                  true, false, false);
    BOOST_CHECK_EQUAL(runner.run("check"),
                      "resolve(1)only1()unsafe1safe1true");
    BOOST_CHECK_EQUAL(runner.run("call_only2"), "only2()");
}
//! \endcond

/*! \file
 * \test \c resolve_replace -- Checks resolved names, with \c replace=true,
 * \c remove=false */
//! \cond
BOOST_AUTO_TEST_CASE(resolve_replace)
{
    test::resolving_runner runner(test::script1, test::script2,
                                  true, true, false);
    BOOST_CHECK_EQUAL(runner.run("check"),
                      "resolve_script2(1)only1()unsafe1safe2PredefReplaced");
    BOOST_CHECK_EQUAL(runner.run("call_only2"), "only2()");
}
//! \endcond

/*! \file
 * \test \c resolve_remove -- Checks resolved names, with \c replace=false,
 * \c remove=true */
//! \cond
BOOST_AUTO_TEST_CASE(resolve_remove)
{
    test::resolving_runner runner(test::script1, test::script2,
                                  true, false, true);
    BOOST_CHECK_EQUAL(runner.run("check"),
                      "resolve(1)only1_dynamic()unsafe1safe1true");
    BOOST_CHECK_EQUAL(runner.run("call_only2"), "only2()");
}
//! \endcond

/*! \file
 * \test \c resolve_replace_remove -- Checks resolved names, with
 * \c replace=true, \c remove=true */
//! \cond
BOOST_AUTO_TEST_CASE(resolve_replace_remove)
{
    test::resolving_runner runner(test::script1, test::script2,
                                  true, true, true);
    BOOST_CHECK_EQUAL(runner.run("check"),
              "resolve_script2(1)only1_dynamic()unsafe1safe2PredefReplaced");
    BOOST_CHECK_EQUAL(runner.run("call_only2"), "only2()");
}
//! \endcond
