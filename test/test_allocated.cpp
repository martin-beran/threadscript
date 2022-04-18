/*! \file
 * \brief Tests of threadscript::deleter, threadscript::unique_ptr_alloc,
 * threadscript::allocate_unique()
 */

//! \cond
#include "threadscript/allocated.hpp"
#include "threadscript/default_allocator.hpp"

#define BOOST_TEST_MODULE allocator_config
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
//! \endcond

/*! \file
 * \test \c unique_ptr_alloc -- Using threadscript::unique_ptr_alloc */
//! \cond
BOOST_AUTO_TEST_CASE(unique_ptr_alloc)
{
    threadscript::allocator_config cfg;
    threadscript::default_allocator<int> alloc{&cfg};
    {
        auto p = threadscript::allocate_unique<int>(alloc, 123);
        auto metrics = cfg.metrics();
        BOOST_TEST(metrics.alloc_ops.load() == 1);
        BOOST_TEST(metrics.alloc_rejects.load() == 0);
        BOOST_TEST(metrics.dealloc_ops.load() == 0);
        BOOST_TEST(metrics.allocs.load() == 1);
        BOOST_TEST(metrics.max_allocs.load() == 1);
        BOOST_TEST(metrics.balance.load() > 0);
        BOOST_TEST(metrics.max_balance.load() == metrics.balance.load());
    }
    auto metrics = cfg.metrics();
    BOOST_TEST(metrics.alloc_ops.load() == 1);
    BOOST_TEST(metrics.alloc_rejects.load() == 0);
    BOOST_TEST(metrics.dealloc_ops.load() == 1);
    BOOST_TEST(metrics.allocs.load() == 0);
    BOOST_TEST(metrics.max_allocs.load() == 1);
    BOOST_TEST(metrics.balance.load() == 0);
    BOOST_TEST(metrics.max_balance.load() > 0);
}
//! \endcond
