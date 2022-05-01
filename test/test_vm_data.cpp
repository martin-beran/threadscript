/* \file
 * \brief Tests of value types.
 * It tests threadscript::basic_value, threadscript::basic_typed_value, and
 * derived value classes for specific data types:
 * \arg threadscript::basic_value_bool
 * \arg threadscript::basic_value_int
 * \arg threadscript::basic_value_uint
 * \arg threadscript::basic_value_string
 */

//! \cond
#include "threadscript/config.hpp"
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE vm_data
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
//! \endcond

/*! \file
 * \test \c value_bool_default -- Default-constructed value_bool */
//! \cond
BOOST_AUTO_TEST_CASE(value_bool_default)
{
    threadscript::allocator_any alloc;
    auto v = threadscript::value_bool::create(alloc);
    BOOST_TEST(v->type_name() == "bool");
    BOOST_TEST(!v->value());
}
//! \endcond
