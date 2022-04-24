#pragma once

/*! \file
 * \brief The default allocator class
 *
 * This file is usually not included. File threadscript/config.hpp is usually
 * included instead and types threadscript::config::allocator_type and
 * threadscript::allocator_any are used as default allocators.
 */

#include "threadscript/config.hpp"

#include <atomic>
#include <locale>
#include <memory>
#include <scoped_allocator>

namespace threadscript {

//! Statistics and limits of an allocator
/*! For better performance, atomic values are used instead of locking, which
 * can make some values imprecise temporarily when read and updated by several
 * threads concurrently.
 * \threadsafe{safe, safe}
 * \test in file test_allocator_config.cpp */
class allocator_config {
public:
    //! Type of counters
    using counter_type = config::counter_type;
    //! A collection of metrics
    /*! \note Keep constructors and assignments consistent when adding data
     * members. */
    struct metrics_t {
        //! Initializes all metrics to zero.
        metrics_t() = default;
        //! Copy constructor
        /*! \param[in] o the the source object */
        // NOLINTNEXTLINE(performance-move-constructor-init)
        metrics_t(const metrics_t& o) noexcept:
            alloc_ops(o.alloc_ops.load(std::memory_order_relaxed)),
            alloc_rejects(o.alloc_rejects.load(std::memory_order_relaxed)),
            dealloc_ops(o.dealloc_ops.load(std::memory_order_relaxed)),
            allocs(o.allocs.load(std::memory_order_relaxed)),
            max_allocs(o.max_allocs.load(std::memory_order_relaxed)),
            balance(o.balance.load(std::memory_order_relaxed)),
            max_balance(o.max_balance.load(std::memory_order_relaxed)) {}
        //! Move constructor is the same as copy.
        /*! \param[in] o the source object */
        // NOLINTNEXTLINE(performance-move-constructor-init)
        metrics_t(metrics_t&& o) noexcept: metrics_t(o) {}
        //! Default destructor
        ~metrics_t() = default;
        //! Copy assignment
        /*! \param[in] o the source object
         * \return \c *this */
        metrics_t& operator=(const metrics_t& o) noexcept {
            if (&o != this) {
                alloc_ops.store(o.alloc_ops.load(std::memory_order_relaxed),
                                std::memory_order_relaxed);
                alloc_rejects.store(o.alloc_rejects.load(
                                                    std::memory_order_relaxed),
                                    std::memory_order_relaxed);
                dealloc_ops.store(o.dealloc_ops.load(std::memory_order_relaxed),
                                  std::memory_order_relaxed);
                allocs.store(o.allocs.load(std::memory_order_relaxed),
                             std::memory_order_relaxed);
                max_allocs.store(o.max_allocs.load(std::memory_order_relaxed),
                                 std::memory_order_relaxed);
                balance.store(o.balance.load(std::memory_order_relaxed),
                              std::memory_order_relaxed);
                max_balance.store(o.max_balance.load(std::memory_order_relaxed),
                                  std::memory_order_relaxed);
            }
            return *this;
        }
        //! Move assignment is the same as copy.
        /*! \param[in] o the source object
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
        std::atomic<size_t> balance = 0;
        //! The maximum size of allocated memory (bytes)
        std::atomic<size_t> max_balance = 0;
    };
    //! A collection of limits
    /*! \note Keep constructors and assignments consistent when adding data
     * members. */
    struct limits_t {
        //! Initializes all limits to unlimited.
        limits_t() = default;
        //! Copy constructor
        /*! \param[in] o source object */
        limits_t(const limits_t& o) noexcept:
            balance(o.balance.load(std::memory_order_relaxed)) {}
        //! Move constructor is the same as copy.
        /*! \param[in] o source object */
        // NOLINTNEXTLINE(performance-move-constructor-init)
        limits_t(limits_t&& o) noexcept: limits_t(o) {}
        //! Default destructor
        ~limits_t() = default;
        //! Copy assignment
        /*! \param[in] o source object
         * \return \c *this */
        limits_t& operator=(const limits_t& o) noexcept {
            if (&o != this) {
                balance.store(o.balance.load(std::memory_order_relaxed),
                              std::memory_order_relaxed);
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
        static constexpr size_t unlimited_size = 0;
        //! Used to denote an unlimited count
        static constexpr counter_type unlimited_count = 0;
        //! The limit of the allocated memory (bytes)
        /*! Zero means unlimited. */
        std::atomic<size_t> balance = unlimited_size;
    };
    //! Checks limits and records an allocation.
    /*! It adjust counters of allocated memory appropriately, adding \a size if
     * returning \c true and not changing them if returning \c false.
     * \param[in] size the size of an allocation
     * \return \c true if the allocation is allowed by limits, \c false if
     * denied due to exceeding a limit */
    bool allocate(size_t size) noexcept;
    //! Records a deallocation.
    /*! It should be eventually called with the same \a size for each
     * allocate() that returned \c true. It must not be called for allocate()
     * that returned \c false.
     * \param[in] size the size of an allocation */
    void deallocate(size_t size) noexcept;
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
/*! It allocates memory by the global <tt>\::operator new()</tt> and
 * deallocates by the global <tt>\::operator delete()</tt>. It provides various
 * metrics about memory allocation and can limit the amount of allocated memory
 * by using allocator_config. Memory limits are not exact, because some
 * overhead may not be included in counters. A single allocator_config object
 * can be shared by multiple allocators.
 *
 * It satisfies requirements of \c std::Allocator, so it is compatible with \c
 * std::allocator_traits and it can be used wherever the standard library
 * expects an allocator.
 * \tparam T the type of allocated objects
 * \tparam CfgPtr the type of a pointer to allocator_config used for metrics
 * and limits of this allocator; a raw or smart pointer to allocator_config
 * \threadsafe{safe, safe}
 * \test in file test_default_allocator.cpp */
template <class T, class CfgPtr = allocator_config*>
class default_allocator: public std::allocator<T> {
    static_assert(std::is_same_v<
        typename std::pointer_traits<CfgPtr>::element_type, allocator_config>);
public:
    //! Enable propagation on container copy
    struct propagate_on_container_copy_assignment: std::true_type {};
    //! Enable propagation on container move
    struct propagate_on_container_move_assignment: std::true_type {};
    //! Enable propagation on container swap
    struct propagate_on_container_swap: std::true_type {};
    //! Creates the allocator
    /*! \param[in] cfg a metrics and limits object; if \c nullptr, metrics will
     * not be recorded and no limits will be enforced */
    explicit constexpr default_allocator(CfgPtr cfg = {}) noexcept:
        _cfg(std::move(cfg)) {}
    //! Copy constructor
    /*! \param[in] o the source object */
    constexpr default_allocator(const default_allocator& o) = default;
    //! Move constructor
    /*! It leaves _cfg in the source object unmodified.
     * \param[in] o the source object */
    constexpr default_allocator(default_allocator&& o) noexcept:
        std::allocator<T>(std::move(o)), _cfg(o._cfg) {}
    //! Rebinding constructor
    /*! \tparam U the value type of the source allocator
     * \param[in] o the source object */
    // NOLINTNEXTLINE(hicpp-explicit-conversions): Implicit conversions used
    template <class U> default_allocator(const default_allocator<U, CfgPtr>& o)
        noexcept: std::allocator<T>(o), _cfg(o._cfg) {}
    //! Default destructor
    constexpr ~default_allocator() = default;
    //! Copy assignment
    /*! \param[in] o the source object
     * \return \c *this */
    constexpr default_allocator& operator=(const default_allocator& o) =
        default;
    //! Move assignment
    /*! It leaves _cfg in the source object unmodified.
     * \param[in] o the source object
     * \return \c *this */
    constexpr default_allocator& operator=(default_allocator&& o) noexcept {
        if (&o != this) {
            std::allocator<T>::operator=(std::move(o));
            _cfg = o._cfg;
        }
        return *this;
    }
    //! Allocation function
    /*! \param[in] n the number of objects to allocate
     * \return a pointer to array of not yet constructed \a n objects
     * \throw std::bad_alloc if allocation is denied by a limit
     * \throw ... any exception thrown by \c std::allocator<T>::allocate() */
    [[nodiscard]] constexpr T* allocate(std::size_t n) {
        if (_cfg && !_cfg->allocate(n * sizeof(T)))
            throw std::bad_alloc();
        try {
            return std::allocator<T>::allocate(n);
        } catch (...) {
            if (_cfg)
                _cfg->deallocate(n * sizeof(T));
            throw;
        }
    }
    //! Deallocation function
    /*! \param[in] p a storage allocated by a previous allocate()
     * \param[in] n the number of objects passed to the related call of
     * allocate() */
    constexpr void deallocate(T* p, std::size_t n) noexcept {
        std::allocator<T>::deallocate(p, n);
        if (_cfg)
            _cfg->deallocate(n * sizeof(T));
    }
    //! Gets the attached allocator_config object.
    /*! \return a metrics and limits object or \c nullptr */
    [[nodiscard]] CfgPtr cfg() const noexcept { return _cfg; }
private:
    CfgPtr _cfg; //!< Metrics and limits, may be \c nullptr
    //! Needed by the rebinding constructor
    template <class U, class CP> friend class default_allocator;
};

//! The default_allocator class with a shared pointer to allocator_config.
/*! \tparam T the type of allocated objects */
template <class T> using default_allocator_shptr =
    default_allocator<T, std::shared_ptr<allocator_config>>;

//! The default_allocator adapted for multilevel standard library containers
/*! \tparam T the type of allocated objects
 * \tparam CfgPtr the type of a pointer to allocator_config used for metrics
 * and limits of this allocator; a raw or smart pointer to allocator_config
 *
 * \sa test_default_allocator.cpp
 * \snippet{lineno} test_default_allocator.cpp Using scoped_allocator */
template <class T, class CfgPtr = allocator_config*> using scoped_allocator =
    std::scoped_allocator_adaptor<default_allocator<T, CfgPtr>>;

//! The scoped_allocator class with a shared pointer to allocator_config.
/*! \tparam T the type of allocated objects */
template <class T> using scoped_allocator_shptr =
    scoped_allocator<T, std::shared_ptr<allocator_config>>;

} // namespace threadscript
