/*! \file
 * \brief Tests of predefined built-in (C++ native) symbols
 *
 * It tests contents of namespace threadscript::predef. Tests should be kept in
 * the order of declarations of tested classes in the namespace (in file
 * predef.hpp).
 */

//! \cond
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE vm_data
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

namespace ts = threadscript;
using namespace std::string_view_literals;
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
