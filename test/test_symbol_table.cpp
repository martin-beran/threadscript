/* \file
 * \brief Tests of class threadscript::basic_symbol_table
 */

//! \cond
#include "threadscript/config.hpp"
#include "threadscript/config_default.hpp"
#include "threadscript/default_allocator.hpp"
#include "threadscript/exception.hpp"
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE symbol_table
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

namespace ts = threadscript;
using namespace std::string_view_literals;
//! \endcond

/*! \file
 * \test \c parent -- Symbol tables with and without a parent. */
//! \cond
BOOST_AUTO_TEST_CASE(parent)
{
    ts::allocator_any alloc;
    ts::symbol_table parent(alloc, nullptr);
    ts::symbol_table child(alloc, &parent);
    BOOST_TEST(parent.parent_table() == nullptr);
    BOOST_TEST(child.parent_table() == &parent);
}
//! \endcond
