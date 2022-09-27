/*! \file
 * \brief Tests of the base class of native C++ objects
 *
 * It tests class threadscript::basic_value_object.
 */

//! \cond
#include "threadscript/threadscript.hpp"
#include "threadscript/symbol_table_impl.hpp"
#include "threadscript/vm_data_impl.hpp"

#define BOOST_TEST_MODULE object
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

namespace ts = threadscript;
using namespace std::string_literals;
using namespace std::string_view_literals;

//! [native_object]
namespace test {

ts::allocator_any alloc;

class empty_object: public ts::value_object<empty_object, "empty_object"> {
    using base::base;
public:
    ~empty_object() {
        ++destroyed;
        if (out)
            *out << "destroyed\n";
    }
    inline static std::ostream *out = nullptr; 
    inline static uintmax_t destroyed = 0;
};

class test_object: public ts::value_object<test_object, "test_object"> {
    using base::base;
public:
    test_object(tag t, std::shared_ptr<const method_table> methods,
                ts::state& thread, ts::symbol_table& l_vars,
                const ts::code_node& node);
private:
    value_ptr dummy(ts::state& thread, ts::symbol_table& l_vars,
                    const ts::code_node& node);
    value_ptr data(ts::state& thread, ts::symbol_table& l_vars,
                   const ts::code_node& node);
    value_ptr throwing(ts::state& thread, ts::symbol_table& l_vars,
                       const ts::code_node& node);
    ts::a_string val;
public:
    static method_table init_methods() {
        return {
            {"dummy", &test_object::dummy},
            {"data", &test_object::data},
            {"throwing", &test_object::throwing},
        };
    }
};

test_object::test_object(tag t, std::shared_ptr<const method_table> methods,
                         ts::state& thread, ts::symbol_table& l_vars,
                         const ts::code_node& node):
    base(t, methods, thread, l_vars, node)
{
    if (narg(node) > 1)
        throw ts::exception::op_library();
    auto a = arg(thread, l_vars, node, 0);
    if (auto v = dynamic_cast<ts::value_string*>(a.get())) {
        val = v->cvalue();
    }
}

auto test_object::data(ts::state& thread, ts::symbol_table& l_vars,
                       const ts::code_node& node) -> value_ptr
{
    auto a = arg(thread, l_vars, node, 1);
    if (auto v = dynamic_cast<ts::value_string*>(a.get())) {
        val = v->cvalue();
        return nullptr;
    } else {
        auto res = ts::value_string::create(thread.get_allocator());
        res->value() = val;
        return res;
    }
}

auto test_object::dummy(ts::state&, ts::symbol_table&,
                        const ts::code_node&) -> value_ptr
{
    return nullptr;
}

auto test_object::throwing(ts::state&, ts::symbol_table&,
                           const ts::code_node&) -> value_ptr
{
    throw ts::exception::op_library();
}

} // namespace test
//! [native_object]

namespace test {

struct script_runner {
    script_runner(std::string script): script(std::move(script)) {
        // Redirect standard output to a string stream
        vm.std_out = &std_out;
        // Register default predefined built-in native commands and functions
        auto sh_vars = ts::predef_symbols(alloc);
        vm.sh_vars = sh_vars;
        // Register native classes
        empty_object::register_constructor(*sh_vars, true);
        test_object::register_constructor(*sh_vars, true);
    }
    ts::value::value_ptr run(bool print_destroys = false);
    ts::value::value_ptr run(ts::state& thread, bool print_destroys = false);
    std::ostringstream std_out;
    ts::virtual_machine vm{alloc};
    std::string script;
};

ts::value::value_ptr script_runner::run(bool print_destroys)
{
    ts::state thread{vm};
    return run(thread, print_destroys);
}

ts::value::value_ptr script_runner::run(ts::state& thread, bool print_destroys)
{
    // Parse the script
    auto parsed = ts::parse_code(alloc, script, "string");
    // Prepare script runtime environment
    if (print_destroys)
        empty_object::out = vm.std_out;
    // Run the script
    auto result = parsed->eval(thread);
    return result;
}

} // namespace test
//! \endcond

/*! \file
 * \test \c object_life_cycle -- Creates new native objects, gets access to an
 * objects by calling them without arguments, checks type names, checks that
 * each constructor call creates a new object, checks destruction of objects. */
//! \cond
BOOST_AUTO_TEST_CASE(create_object)
{
    test::script_runner runner(R"(
        seq(
            gvar("ge", empty_object()),
            var("e1", empty_object()),
            var("e2", empty_object()),
            print(type(e1()), ",", type(e2()), "\n"),
            print(is_same(e1(), e2()), "\n"),
            var("e2", null),
            print("end\n")
        )
    )");
    {
        ts::state thread{runner.vm};
        BOOST_CHECK_EQUAL(runner.run(thread, true), nullptr);
        BOOST_CHECK_EQUAL(runner.std_out.view(),
                          "empty_object,empty_object\n"
                          "false\n"
                          "destroyed\n" // e2
                          "end\n"
                          "destroyed\n"); // e1
        BOOST_CHECK_EQUAL(test::empty_object::destroyed, 2);
    }
    // global variables destroyed with state
    BOOST_CHECK_EQUAL(test::empty_object::destroyed, 3); // destroyed ge
}
//! \endcond

/*! \file
 * \test \c object_methods -- Calling methods of objects. It also demonstrates
 * saving a value from script in the thread-local symbol table for later use by
 * the calling C++ native code. */
//! \cond
BOOST_AUTO_TEST_CASE(object_methods)
{
    test::script_runner runner(R"(
        seq(
            var("o1", test_object()),
            var("o2", test_object("constructor_arg")),
            print("dummy: ", o1("dummy"), "\n"),
            print("get: ", o1("data"), ",", o2("data"), "\n"),
            o1("data", "abcdef"),
            o2("data", "XYZ"),
            print("get: ", o1("data"), ",", o2("data"), "\n"),
            gvar("data1", o1("data"))
        )
    )");
    ts::state thread{runner.vm};
    BOOST_CHECK_EQUAL(runner.run(thread), nullptr);
    BOOST_CHECK_EQUAL(runner.std_out.view(),
                      "dummy: null\n"
                      "get: ,constructor_arg\n"
                      "get: abcdef,XYZ\n");
    auto v = thread.t_vars.lookup("data1");
    BOOST_REQUIRE(v.has_value());
    auto pv = v.value();
    BOOST_CHECK(pv);
    auto ps = dynamic_cast<ts::value_string*>(pv.get());
    BOOST_CHECK(ps);
    BOOST_CHECK_EQUAL(ps->cvalue(), "abcdef");
}
//! \endcond

/*! \file
 * \test \c exception_constructor -- Throwing an exception from an object
 * constructor. */
//! \cond
BOOST_AUTO_TEST_CASE(exception_constructor)
{
    test::script_runner runner(R"(
        try(
            var("o", test_object("a", 2)),
            "op_library", print("op_library exception")
        )
    )");
    BOOST_REQUIRE_NO_THROW(runner.run());
    BOOST_CHECK_EQUAL(runner.std_out.view(), "op_library exception");
}
//! \endcond

/*! \file
 * \test \c exception_method_null -- Throwing an exception if calling with \c
 * null method name. */
//! \cond
BOOST_AUTO_TEST_CASE(exception_method_null)
{
    test::script_runner runner(R"(
        try(
            seq(
                var("o", test_object("a")),
                o(null)
            ),
            "value_null", print("value_null exception")
        )
    )");
    BOOST_REQUIRE_NO_THROW(runner.run());
    BOOST_CHECK_EQUAL(runner.std_out.view(), "value_null exception");
}
//! \endcond

/*! \file
 * \test \c exception_method_type -- Throwing an exception if calling with
 * method name argument not being a \c string. */
//! \cond
BOOST_AUTO_TEST_CASE(exception_method_type)
{
    test::script_runner runner(R"(
        try(
            seq(
                var("o", test_object("a")),
                o(1)
            ),
            "value_type", print("value_type exception")
        )
    )");
    BOOST_REQUIRE_NO_THROW(runner.run());
    BOOST_CHECK_EQUAL(runner.std_out.view(), "value_type exception");
}
//! \endcond

/*! \file
 * \test \c exception_method_unknown -- Throwing an exception if calling an
 * unknown method name. */
//! \cond
BOOST_AUTO_TEST_CASE(exception_method_unknown)
{
    test::script_runner runner(R"(
        try(
            seq(
                var("o", test_object("a")),
                o("nonexistent")
            ),
            "not_implemented", seq(
                print("not_implemented exception"),
                throw()
            )
        )
    )");
    BOOST_CHECK_EXCEPTION(runner.run(), ts::exception::not_implemented,
        ([](auto&& e) {
            BOOST_CHECK_EQUAL(e.what(),
                              "string:5:17:(): nonexistent not implemented");
            return true;
         }));
    BOOST_CHECK_EQUAL(runner.std_out.view(), "not_implemented exception");
}
//! \endcond

/*! \file
 * \test \c exception_method_call -- Passing an exception thrown by a method
 * implementation. */
//! \cond
BOOST_AUTO_TEST_CASE(exception_method_call)
{
    test::script_runner runner(R"(
        try(
            seq(
                var("o", test_object("a")),
                o("throwing")
            ),
            "op_library", print("op_library exception")
        )
    )");
    BOOST_REQUIRE_NO_THROW(runner.run());
    BOOST_CHECK_EQUAL(runner.std_out.view(), "op_library exception");
}
//! \endcond

/*! \file
 * \test \c exception_pass -- Passing an unhandled exception from a method to
 * the calling native C++ code. */
//! \cond
BOOST_AUTO_TEST_CASE(exception_pass)
{
    test::script_runner runner(R"(
        seq(
            var("o", test_object("a")),
            o("throwing")
        )
    )");
    BOOST_CHECK_THROW(runner.run(), ts::exception::op_library);
}
//! \endcond
