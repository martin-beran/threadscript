/*! \file
 * \brief Tests of template threadscript::default_allocator
 */

//! \cond
#include "threadscript/config.hpp"
#include "threadscript/default_allocator.hpp"

#include <scoped_allocator>

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
    BOOST_TEST(a.cfg() == nullptr);
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
    BOOST_TEST(a.cfg() == &cfg);
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
    BOOST_TEST(a.cfg() == nullptr);
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
    BOOST_TEST(a.cfg().get() == cfg.get());
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
 */
//! \cond
BOOST_AUTO_TEST_CASE(limits)
{
    threadscript::allocator_config cfg;
    threadscript::allocator_config::limits_t limits;
    limits.balance = 1024;
    cfg.limits(limits);
    threadscript::default_allocator<uint32_t> a(&cfg);
    // allocate successful
    const size_t sz0 = 4;
    auto p0 = a.allocate(sz0);
    BOOST_TEST(p0 != nullptr);
    auto metrics = a.cfg()->metrics();
    BOOST_TEST(metrics.alloc_ops.load() == 1);
    BOOST_TEST(metrics.alloc_rejects.load() == 0);
    BOOST_TEST(metrics.dealloc_ops.load() == 0);
    BOOST_TEST(metrics.allocs.load() == 1);
    BOOST_TEST(metrics.max_allocs.load() == 1);
    BOOST_TEST(metrics.balance.load() == sz0 * sizeof(uint32_t));
    BOOST_TEST(metrics.max_balance.load() == sz0 * sizeof(uint32_t));
    // deallocate
    a.deallocate(p0, sz0);
    metrics = a.cfg()->metrics();
    BOOST_TEST(metrics.alloc_ops.load() == 1);
    BOOST_TEST(metrics.alloc_rejects.load() == 0);
    BOOST_TEST(metrics.dealloc_ops.load() == 1);
    BOOST_TEST(metrics.allocs.load() == 0);
    BOOST_TEST(metrics.max_allocs.load() == 1);
    BOOST_TEST(metrics.balance.load() == 0);
    BOOST_TEST(metrics.max_balance.load() == sz0 * sizeof(uint32_t));
    // allocate successful
    p0 = a.allocate(sz0);
    BOOST_TEST(p0 != nullptr);
    // allocate failure
    const size_t sz1 = 2048;
    BOOST_CHECK_THROW((void)a.allocate(sz1), std::bad_alloc);
    metrics = a.cfg()->metrics();
    BOOST_TEST(metrics.alloc_ops.load() == 2);
    BOOST_TEST(metrics.alloc_rejects.load() == 1);
    BOOST_TEST(metrics.dealloc_ops.load() == 1);
    BOOST_TEST(metrics.allocs.load() == 1);
    BOOST_TEST(metrics.max_allocs.load() == 1);
    BOOST_TEST(metrics.balance.load() == sz0 * sizeof(uint32_t));
    BOOST_TEST(metrics.max_balance.load() == sz0 * sizeof(uint32_t));
    // allocate successful
    const size_t sz2 = 100;
    auto p2 = a.allocate(sz2);
    BOOST_TEST(p2 != nullptr);
    metrics = a.cfg()->metrics();
    BOOST_TEST(metrics.alloc_ops.load() == 3);
    BOOST_TEST(metrics.alloc_rejects.load() == 1);
    BOOST_TEST(metrics.dealloc_ops.load() == 1);
    BOOST_TEST(metrics.allocs.load() == 2);
    BOOST_TEST(metrics.max_allocs.load() == 2);
    BOOST_TEST(metrics.balance.load() >= (sz0 + sz2) * sizeof(uint32_t));
    BOOST_TEST(metrics.max_balance.load() == metrics.balance.load());
    // deallocate
    a.deallocate(p0, sz0);
    a.deallocate(p2, sz2);
    metrics = a.cfg()->metrics();
    BOOST_TEST(metrics.alloc_ops.load() == 3);
    BOOST_TEST(metrics.alloc_rejects.load() == 1);
    BOOST_TEST(metrics.dealloc_ops.load() == 3);
    BOOST_TEST(metrics.allocs.load() == 0);
    BOOST_TEST(metrics.max_allocs.load() == 2);
    BOOST_TEST(metrics.balance.load() == 0);
    BOOST_TEST(metrics.max_balance.load() >= (sz0 + sz2) * sizeof(uint32_t));
}
//! \endcond

/*! \file
 * \test \c std_use_allocator -- Use threadscript::default_allocator in a
 * standard library class (\c std::basic_string) that expects an allocator */
//! \cond
BOOST_AUTO_TEST_CASE(std_use_allocator)
{
    namespace ts = threadscript;
    using allocator_t = ts::default_allocator<char>;
    ts::allocator_config cfg;
    {
        std::basic_string<char, std::char_traits<char>, allocator_t>
            str{allocator_t{&cfg}};
        BOOST_TEST(str.get_allocator().cfg() == &cfg);
        for (char c = 'A'; c <= 'Z'; ++c)
            str += c;
    }
    auto metrics = cfg.metrics();
    BOOST_TEST(metrics.alloc_ops.load() > 0);
    BOOST_TEST(metrics.alloc_rejects.load() == 0);
    BOOST_TEST(metrics.dealloc_ops.load() == metrics.alloc_ops.load());
    BOOST_TEST(metrics.allocs.load() == 0);
    BOOST_TEST(metrics.max_allocs.load() > 0);
    BOOST_TEST(metrics.balance.load() == 0);
    BOOST_TEST(metrics.max_balance.load() >= 0);
}
//! \endcond

/*! \file
 * \test \c std_nested_container -- Use threadscript::default_allocator in a
 * multilevel standard library container. The inner container uses a
 * default-constructed allocator */
//! \cond
BOOST_AUTO_TEST_CASE(std_nested_container)
{
    namespace ts = threadscript;
    ts::allocator_config cfg;
    using allocator1 = ts::default_allocator<int>;
    using container1 = std::vector<int, allocator1>;
    using allocator0 = ts::default_allocator<container1>;
    using container0 = std::vector<container1, allocator0>;
    {
        container0 v{allocator0{&cfg}};
        BOOST_TEST(v.get_allocator().cfg() == &cfg);
        v.resize(1);
        BOOST_TEST(v[0].get_allocator().cfg() == nullptr);
    }
    auto metrics = cfg.metrics();
    BOOST_TEST(metrics.alloc_ops.load() == 1);
    BOOST_TEST(metrics.alloc_rejects.load() == 0);
    BOOST_TEST(metrics.dealloc_ops.load() == metrics.alloc_ops.load());
    BOOST_TEST(metrics.allocs.load() == 0);
    BOOST_TEST(metrics.max_allocs.load() == 1);
    BOOST_TEST(metrics.balance.load() == 0);
    BOOST_TEST(metrics.max_balance.load() >= 0);
}
//! \endcond

/*! \file
 * \test \c std_scoped_allocator -- Use threadscript::default_allocator wrapped
 * in a \c std::scoped_allocator_adaptor in a multilevel standard library
 * container. The inner container uses an allocator with the same configuration
 * as the outer container. */
//! \cond
BOOST_AUTO_TEST_CASE(std_scoped_allocator)
{
//! [Using scoped_allocator]
    namespace ts = threadscript;
    ts::allocator_config cfg;
    // Alternative: using allocator2 = ts::default_allocator<int>;
    using allocator2 = ts::scoped_allocator<int>;
    using container2 = std::vector<int, allocator2>;
    using allocator1 = ts::scoped_allocator<container2>;
    using container1 = std::vector<container2, allocator1>;
    using scoped0 = ts::scoped_allocator<container1>;
    using container0 = std::vector<container1, scoped0>;
    {
        container0 v{scoped0{&cfg}};
//! [Using scoped_allocator]
        BOOST_TEST(v.get_allocator().cfg() == &cfg);
        v.resize(1);
        v[0].resize(1);
        BOOST_TEST(v[0].get_allocator().cfg() == &cfg);
        BOOST_TEST(v[0][0].get_allocator().cfg() == &cfg);
    }
    auto metrics = cfg.metrics();
    BOOST_TEST(metrics.alloc_ops.load() == 2);
    BOOST_TEST(metrics.alloc_rejects.load() == 0);
    BOOST_TEST(metrics.dealloc_ops.load() == metrics.alloc_ops.load());
    BOOST_TEST(metrics.allocs.load() == 0);
    BOOST_TEST(metrics.max_allocs.load() == 2);
    BOOST_TEST(metrics.balance.load() == 0);
    BOOST_TEST(metrics.max_balance.load() >= 0);
}
//! \endcond
