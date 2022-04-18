#pragma once

/*! \file
 * \brief Declarations that use an allocator, e.g., an allocator-aware unique
 * pointer.
 */

#include <memory>

namespace threadscript {

//! An allocater-aware replacement of \c std::default_delete
/*! \tparam T the allocated type
 * \tparam Allocator an allocator type
 * \tparam U the type allocated by \a Allocator
 * \test in file test_allocated.cpp */
template <class T, class Allocator> class deleter {
public:
    //! Stores an allocator
    /*! \param[in] alloc the allocator used by this deleter */
    explicit deleter(Allocator alloc) noexcept: alloc(std::move(alloc)) {}
    //! Deletes an object using the stored allocator.
    /*! It does nothing if \a p is \c nullptr
     * \param[in] p an object to be deleted */
    void operator()(T* p) noexcept {
        if (p)
            alloc.deallocate(p, 1);
    }
private:
    Allocator alloc; //!< The stored allocator
};

//! An allocater-aware replacement of \c std::unique_ptr
/*! Instances are usually created by allocate_unique().
 * \tparam T the allocated type
 * \tparam Allocator an allocator template
 * \tparam U the type allocated by \a Allocator
 * \test in file test_allocated.cpp */
template <class T, class Allocator>
using unique_ptr_alloc = std::unique_ptr<T, deleter<T, Allocator>>;

//! A function to allocate an object and create unique_ptr_alloc.
/*! \tparam T the allocated type
 * \tparam Allocator an allocator template
 * \tparam U the type allocated by \a Allocator
 * \tparam Args constructor parameters of \a T
 * \param[in] alloc an allocator to use for allocation and deallocation
 * \param[in] args arguments for a constructor of T
 * \return a unique_ptr_alloc pointing to the allocated object
 * \test in file test_allocated.cpp */
template <class T, class Allocator, class ...Args>
auto allocate_unique(const Allocator& alloc, Args&& ...args)
{
    using a_t =
        typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
    a_t a{alloc};
    T* p = new(a.allocate(1)) T(std::forward<Args>(args)...);
    return
        unique_ptr_alloc<T, a_t>(p, deleter<T, a_t>(std::move(alloc)));
}

} // namespace threadscript
