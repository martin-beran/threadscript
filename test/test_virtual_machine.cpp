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

/*! \file
 * \test \c vm_sh_vars -- Manipulating shared global symbol table of a VM */
//! \cond
BOOST_AUTO_TEST_CASE(vm_sh_vars)
{
    threadscript::allocator_any alloc;
    threadscript::virtual_machine vm(alloc);
    BOOST_CHECK(!vm.sh_vars.load());
    auto vars1 = std::make_shared<threadscript::symbol_table>(alloc, nullptr);
    vm.sh_vars = vars1;
    BOOST_CHECK_EQUAL(vars1.use_count(), 2);
    threadscript::state state(vm);
    BOOST_CHECK_EQUAL(vars1.use_count(), 3);
    BOOST_CHECK_EQUAL(vm.sh_vars.load().get(), state.t_vars.parent_table());
    BOOST_TEST(vm.num_states() == 1);
    BOOST_TEST(!state.t_vars.contains("var"));
    auto vars2 = std::make_shared<threadscript::symbol_table>(alloc, nullptr);
    vars2->insert("var", nullptr);
    vm.sh_vars = vars2;
    BOOST_CHECK_EQUAL(vars1.use_count(), 2);
    BOOST_CHECK_EQUAL(vars2.use_count(), 2);
    BOOST_CHECK_NE(vm.sh_vars.load().get(), state.t_vars.parent_table());
    BOOST_TEST(!state.t_vars.contains("var"));
    state.update_sh_vars();
    BOOST_CHECK_EQUAL(vars2.use_count(), 3);
    BOOST_CHECK_EQUAL(vm.sh_vars.load().get(), state.t_vars.parent_table());
    BOOST_TEST(state.t_vars.contains("var"));
}
//! \endcond
