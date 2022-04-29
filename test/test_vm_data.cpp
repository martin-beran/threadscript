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
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE vm_data
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
//! \endcond
