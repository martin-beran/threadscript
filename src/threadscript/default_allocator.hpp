#pragma once

/*! \file
 * \brief The default allocator class
 */

#include "threadscript/config.hpp"
#include "threadscript/config_default.hpp"

#include <atomic>

namespace threadscript {

//! Statistics and limits of an allocator
/*! For better performance, atomic values are used instead of locking, which
 * can make some values imprecise temporarily when read and updated by several
 * threads concurrently.
 *
 * This class is thread-safe, a single instance can be accessed by multiple
 * threads simultanously. */
class allocator_config {
public:
    //! Type of allocation sizes
    using size_type = config::size_type;
    //! Type of counters
    using counter_type = config::counter_type;
    //! A collection of metrics
    /*! \note Keep constructors and assignments consistent when adding data
     * members. */
    struct metrics_t {
        //! Initializes all metrics to zero.
        metrics_t() = default;
        //! Copy constructor
        /*! \param[in] o source object */
        metrics_t(const metrics_t& o) noexcept:
            alloc_ops(o.alloc_ops.load()),
            alloc_rejects(o.alloc_rejects.load()),
            dealloc_ops(o.dealloc_ops.load()),
            allocs(o.allocs.load()), max_allocs(o.max_allocs.load()),
            balance(o.balance.load()), max_balance(o.max_balance.load()) {}
        //! Move constructor is the same as copy.
        /*! \param[in] o source object */
        metrics_t(metrics_t&& o) noexcept: metrics_t(o) {}
        //! Default destructor
        ~metrics_t() = default;
        //! Copy assignment
        /*! \param[in] o source object
         * \return \c *this */
        metrics_t& operator=(const metrics_t& o) noexcept {
            if (&o != this) {
                alloc_ops = o.alloc_ops.load();
                alloc_rejects = o.alloc_rejects.load();
                dealloc_ops = o.dealloc_ops.load();
                allocs = o.allocs.load();
                max_allocs = o.max_allocs.load();
                balance = o.balance.load();
                max_balance = o.max_balance.load();
            }
            return *this;
        }
        //! Move assignment is the same as copy.
        /*! \param[in] o source object
         * \return \c *this */
        metrics_t& operator=(metrics_t&& o) noexcept {
            operator=(o);
            return *this;
        }
        //! The number of successful allocation operations
        std::atomic<counter_type> alloc_ops = 0;
        //! The number of allocations operations rejected by limits
        std::atomic<counter_type> alloc_rejects = 0;
        //! The number of deallocation operations
        std::atomic<counter_type> dealloc_ops = 0;
        //! The current number of allocated objects
        std::atomic<counter_type> allocs = 0;
        //! The maximum number of allocated objects
        std::atomic<counter_type> max_allocs = 0;
        //! The current size of allocated memory (bytes)
        std::atomic<size_type> balance = 0;
        //! The maximum size of allocated memory (bytes)
        std::atomic<size_type> max_balance = 0;
    };
    //! A collection of limits
    /*! \note Keep constructors and assignments consistent when adding data
     * members. */
    struct limits_t {
        //! Initializes all limits to unlimited.
        limits_t() = default;
        //! Copy constructor
        /*! \param[in] o source object */
        limits_t(const limits_t& o) noexcept: balance(o.balance.load()) {}
        //! Move constructor is the same as copy.
        /*! \param[in] o source object */
        limits_t(limits_t&& o) noexcept: limits_t(o) {}
        //! Default destructor
        ~limits_t() = default;
        //! Copy assignment
        /*! \param[in] o source object
         * \return \c *this */
        limits_t& operator=(const limits_t& o) noexcept {
            if (&o != this) {
                balance = o.balance.load();
            }
            return *this;
        }
        //! Move assignment is the same as copy.
        /*! \param[in] o source object
         * \return \c *this */
        limits_t& operator=(limits_t&& o) noexcept {
            operator=(o);
            return *this;
        }
        //! Used to denote an unlimited size
        static constexpr size_type unlimited_size = 0;
        //! Used to denote an unlimited count
        static constexpr counter_type unlimited_count = 0;
        //! The limit of the allocated memory (bytes)
        /*! Zero means unlimited. */
        std::atomic<size_type> balance = unlimited_size;
    };
    //! Checks limits and records an allocation.
    /*! It adjust counters of allocated memory appropriately, adding \a size if
     * returning \c true and not changing them if returning \c false.
     * \param[in] size the size of an allocation
     * \return \c true if the allocation is allowed by limits, \c false if
     * denied due to exceeding a limit */
    bool allocate(size_type size) noexcept;
    //! Records a deallocation.
    /*! It should be eventually called with the same \a size for each
     * allocate() that returned \c true. It must not be called for allocate()
     * that returned \c false.
     * \param[in] size the size of an allocation */
    void deallocate(size_type size) noexcept;
    //! Gets the metrics.
    /*! \return the current values of metrics */
    [[nodiscard]] metrics_t metrics() const noexcept { return _metrics; }
    //! Gets the limits.
    /*! \return the current values of limits */
    [[nodiscard]] limits_t limits() const noexcept { return _limits; }
    //! Sets new limits.
    /*! \param[in] l the new limits */
    void limits(const limits_t& l) noexcept { _limits = l; }
private:
    metrics_t _metrics; //!< The current values of metrics
    limits_t _limits; //!< The current values of limits
};

//! The default allocator class.
/*! It provides various statistics about memory allocation and can limit the
 * amount of allocated memory. Memory limits are not exact, because some
 * overhead may not be included in counters.
 *
 * It satisfies requirements of \c std::Allocator, so it is compatible with \c
 * std::allocator_traits and it can be used wherever the standard library
 * expects an allocator. */
template <class T> class default_allocator {
public:
    using value_type = T; //!< The type of allocated values
};

} // namespace threadscript
