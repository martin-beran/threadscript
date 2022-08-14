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

#include <sstream>

//! [parse_run]
namespace ts = threadscript;
using namespace std::string_view_literals;

namespace test {

struct script_runner {
    script_runner(std::string script): script(std::move(script)) {
        // Redirect standard output to a string stream
        vm.std_out = &std_out;
        // Register default predefined built-in native commands and functions
        vm.sh_vars = ts::predef_symbols(alloc);
    }
    ts::value::value_ptr run();
    ts::allocator_any alloc;
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
    ts::symbol_table lookup{alloc};
    // Run the script
    auto result = parsed->eval(thread, lookup, sym);
    return result;
}

} // namespace test
//! [parse_run]
//! \endcond

/*! \file
 * \test \c print -- Test of threadscript::predef::f_print */
//! \cond
BOOST_AUTO_TEST_CASE(print)
{
    BOOST_CHECK(true);
    // TODO
}
//! \endcond
