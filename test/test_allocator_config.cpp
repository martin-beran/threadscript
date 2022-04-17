/*! \file
 * \brief Tests of class threadscript::allocator_config
 */

//! \cond
#include "threadscript/default_allocator.hpp"

#include <optional>
#include <type_traits>
#define BOOST_TEST_MODULE allocator_config
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/size.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

namespace test {

template <class T> class vector: public std::vector<T> {
    using std::vector<T>::vector;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const test::vector<T>& v)
{
    os << '{';
    size_t i = 0;
    for (const auto& e: v) {
        if (i > 0)
            os << ',';
        if (i >= 10) {
            os << "...";
            break;
        }
        ++i;
        os << e;
    }
    os << '}';
    return os;
}

} // namespace test
//! \endcond

/*! \file
 * \test \c default_constructed -- Metrics in a default-constructed
 * threadscript::allocator_config are zero and limits are unlimited. */
//! \cond
BOOST_AUTO_TEST_CASE(default_constructed)
{
    threadscript::allocator_config cfg;
    auto metrics = cfg.metrics();
    auto limits = cfg.limits();
    BOOST_TEST(metrics.alloc_ops == 0);
    BOOST_TEST(metrics.alloc_rejects == 0);
    BOOST_TEST(metrics.dealloc_ops == 0);
    BOOST_TEST(metrics.allocs == 0);
    BOOST_TEST(metrics.max_allocs == 0);
    BOOST_TEST(metrics.balance == 0);
    BOOST_TEST(metrics.max_balance == 0);
    BOOST_TEST(limits.balance == limits.unlimited_size);
}
//! \endcond

/*! \file
 * \test \c set_limits -- Limits are remembered correctly */
//! \cond
BOOST_AUTO_TEST_CASE(set_limits)
{
    threadscript::allocator_config cfg;
    threadscript::allocator_config::limits_t limits;
    limits.balance = 4096;
    cfg.limits(limits);
    auto l = cfg.limits();
    BOOST_TEST(l.balance == 4096);
}
//! \endcond

/*! \file
 * \test \c alloc_success -- Metrics of successful allocations */
//! \cond
BOOST_DATA_TEST_CASE(alloc_success,
                     (test::vector<test::vector<int>>{
                         { 1, 8, 20, 64 },
                         { 1, -1, 8, 20, -8, 64, -64, -20 },
                     }))
{
    threadscript::allocator_config cfg;
    uint64_t alloc_ops = 0;
    uint64_t dealloc_ops = 0;
    uint64_t allocs = 0;
    uint64_t max_allocs = 0;
    uint64_t balance = 0;
    uint64_t max_balance = 0;
    size_t i = 0;
    for (auto sz: sample) {
        BOOST_TEST_CONTEXT("i=" << i) {
            if (sz >= 0) {
                BOOST_TEST(cfg.allocate(sz));
                ++alloc_ops;
                ++allocs;
            } else {
                cfg.deallocate(-sz);
                ++dealloc_ops;
                --allocs;
            }
            balance += sz;
            max_allocs = std::max(max_allocs, allocs);
            max_balance = std::max(max_balance, balance);
            auto metrics = cfg.metrics();
            BOOST_TEST(metrics.alloc_ops.load() == alloc_ops);
            BOOST_TEST(metrics.alloc_rejects.load() == 0);
            BOOST_TEST(metrics.dealloc_ops.load() == dealloc_ops);
            BOOST_TEST(metrics.allocs.load() == allocs);
            BOOST_TEST(metrics.max_allocs.load() == max_allocs);
            BOOST_TEST(metrics.balance.load() == balance);
            BOOST_TEST(metrics.max_balance.load() == max_balance);
        }
        ++i;
    }
}
//! \endcond

/*! \file
 * \test \c alloc_reject -- Rejecting allocations due to limits */
//! \cond
BOOST_DATA_TEST_CASE(alloc_reject,
                     (test::vector<test::vector<int>>{
                         { 2, 4, 8, 16 },
                         { 1023 }, 
                         { 1024 }, 
                         { 1025 }, 
                         { 1000, 23, 1, 1 },
                         { 400, 500 , 600, -500, 600 },
                         { 400, 500, 600, 111 },
                         { 400, 500 , 1000, -500, 1000, -400, 1000 },
                      }))
{
    threadscript::allocator_config cfg;
    threadscript::allocator_config::limits_t limits;
    limits.balance = 1024;
    cfg.limits(limits);
    uint64_t alloc_ops = 0;
    uint64_t alloc_rejects = 0;
    uint64_t dealloc_ops = 0;
    uint64_t allocs = 0;
    uint64_t max_allocs = 0;
    uint64_t balance = 0;
    uint64_t max_balance = 0;
    size_t i = 0;
    for (auto sz: sample) {
        BOOST_TEST_CONTEXT("i=" << i) {
            std::optional<bool> ok;
            if (sz >= 0) {
                ok = cfg.allocate(sz);
                if (balance + sz <= limits.balance) {
                    BOOST_TEST(*ok);
                    ++alloc_ops;
                    ++allocs;
                    balance += sz;
                } else {
                    BOOST_TEST(!*ok);
                    ++alloc_rejects;
                }
            } else {
                cfg.deallocate(-sz);
                ++dealloc_ops;
                --allocs;
                balance += sz;
            }
            max_allocs = std::max(max_allocs, allocs);
            max_balance = std::max(max_balance, balance);
            auto metrics = cfg.metrics();
            BOOST_TEST(metrics.alloc_ops.load() == alloc_ops);
            BOOST_TEST(metrics.alloc_rejects.load() == alloc_rejects);
            BOOST_TEST(metrics.dealloc_ops.load() == dealloc_ops);
            BOOST_TEST(metrics.allocs.load() == allocs);
            BOOST_TEST(metrics.max_allocs.load() == max_allocs);
            BOOST_TEST(metrics.balance.load() == balance);
            BOOST_TEST(metrics.max_balance.load() == max_balance);
        }
        ++i;
    }
}
//! \endcond
