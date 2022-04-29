/*! \file
 * \brief Tests of threadscript::basic_virtual_machine,
 * threadscript::basic_state, threadscript::virtual_machine,
 * threadscript::state
 */

//! \cond
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE virtual_machine
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
//! \endcond

/*! \file
 * \test \c vm_states -- Creating and destroying a VM and related states */
//! \cond
BOOST_AUTO_TEST_CASE(vm_states)
{
    threadscript::allocator_any alloc;
    threadscript::virtual_machine vm(alloc);
    BOOST_TEST(vm.num_states() == 0);
    {
        threadscript::state state0(vm);
        BOOST_TEST(vm.num_states() == 1);
        {
            threadscript::state state1(vm);
            BOOST_TEST(vm.num_states() == 2);
        }
        BOOST_TEST(vm.num_states() == 1);
    }
    BOOST_TEST(vm.num_states() == 0);
}
//! \endcond
