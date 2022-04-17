/*! \file
 * \brief Tests of template threadscript::default_allocator
 */

//! \cond
#include "threadscript/default_allocator.hpp"

#define BOOST_TEST_MODULE allocator_config
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
//! \endcond

/*! \file
 * \test \c no_cfg -- Class threadscript::default_allocator works without an
 * attached threadscript::allocator_config. */
//! \cond
BOOST_AUTO_TEST_CASE(no_cfg)
{
    threadscript::default_allocator<uint32_t> a;
    for (int n: {1, 2, 4, 8}) {
        BOOST_TEST_CONTEXT("n=" << n) {
            uint32_t* p = a.allocate(n);
            BOOST_TEST(p != nullptr);
            a.deallocate(p, n);
        }
    }
}
//! \endcond

/*! \file
 * \test \c cfg -- Class threadscript::default_allocator works with an attached
 * threadscript::allocator_config. */
//! \cond
BOOST_AUTO_TEST_CASE(cfg)
{
    threadscript::allocator_config cfg;
    threadscript::default_allocator<uint32_t> a(&cfg);
    uint64_t ops = 0;
    for (int n: {1, 2, 4, 8}) {
        BOOST_TEST_CONTEXT("n=" << n) {
            uint32_t* p = a.allocate(n);
            BOOST_TEST(p != nullptr);
            uint64_t balance = cfg.metrics().balance.load();
            a.deallocate(p, n);
            ++ops;
            auto metrics = a.cfg()->metrics();
            BOOST_TEST(metrics.alloc_ops.load() == ops);
            BOOST_TEST(metrics.alloc_rejects.load() == 0);
            BOOST_TEST(metrics.dealloc_ops.load() == ops);
            BOOST_TEST(metrics.allocs.load() == 0);
            BOOST_TEST(metrics.max_allocs.load() == 1);
            BOOST_TEST(metrics.balance.load() == 0);
            BOOST_TEST(metrics.max_balance.load() == balance);
        }
    }
}
//! \endcond

/*! \file
 * \test \c no_cfg_shptr -- Class threadscript::default_allocator_shptr works
 * without an attached threadscript::allocator_config. */
//! \cond
BOOST_AUTO_TEST_CASE(no_cfg_shptr)
{
    threadscript::default_allocator_shptr<uint32_t> a;
    for (int n: {1, 2, 4, 8}) {
        BOOST_TEST_CONTEXT("n=" << n) {
            uint32_t* p = a.allocate(n);
            BOOST_TEST(p != nullptr);
            a.deallocate(p, n);
        }
    }
}
//! \endcond

/*! \file
 * \test \c cfg_shptr -- Class threadscript::default_allocator_shptr works with
 * an attached threadscript::allocator_config. */
//! \cond
BOOST_AUTO_TEST_CASE(cfg_shptr)
{
    auto cfg = std::make_shared<threadscript::allocator_config>();
    threadscript::default_allocator_shptr<uint32_t> a(cfg);
    uint64_t ops = 0;
    for (int n: {1, 2, 4, 8}) {
        BOOST_TEST_CONTEXT("n=" << n) {
            uint32_t* p = a.allocate(n);
            BOOST_TEST(p != nullptr);
            uint64_t balance = cfg->metrics().balance.load();
            a.deallocate(p, n);
            ++ops;
            auto metrics = a.cfg()->metrics();
            BOOST_TEST(metrics.alloc_ops.load() == ops);
            BOOST_TEST(metrics.alloc_rejects.load() == 0);
            BOOST_TEST(metrics.dealloc_ops.load() == ops);
            BOOST_TEST(metrics.allocs.load() == 0);
            BOOST_TEST(metrics.max_allocs.load() == 1);
            BOOST_TEST(metrics.balance.load() == 0);
            BOOST_TEST(metrics.max_balance.load() == balance);
        }
    }
}
//! \endcond

/*! \file
 * \test \c limits -- Class threadscript::default_allocator works with an
 * attached threadscript::allocator_config and according to configured limits.
 * */
//! \cond
BOOST_AUTO_TEST_CASE(limits)
{
    threadscript::allocator_config cfg;
    threadscript::default_allocator<uint32_t> a(&cfg);
    BOOST_TEST(false); // TODO: implement the test
}
//! \endcond
