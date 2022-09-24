/*! \file
 * \brief Tests of the base class of native C++ objects
 *
 * It tests class threadscript::basic_value_object.
 */

//! \cond
#include "threadscript/threadscript.hpp"
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
 * \test \c object_life_cycle -- Creates new native objects, checks type names,
 * checks that each constructor call creates a new object, checks destruction
 * of objects. */
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
