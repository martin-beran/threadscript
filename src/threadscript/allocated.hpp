#pragma once

/*! \file
 * \brief Declarations that use an allocator, e.g., an allocator-aware unique
 * pointer.
 */

#include "threadscript/concepts.hpp"

#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace threadscript {

//! An allocater-aware replacement of \c std::default_delete
/*! \tparam T the allocated type
 * \tparam A an allocator type
 * \tparam U the type allocated by \a A
 * \test in file test_allocated.cpp */
template <class T, impl::allocator A> class deleter {
public:
    //! Stores an allocator
    /*! \param[in] alloc the allocator used by this deleter */
    explicit deleter(const A& alloc) noexcept: alloc(alloc) {}
    //! Deletes an object using the stored allocator.
    /*! It does nothing if \a p is \c nullptr
     * \param[in] p an object to be deleted */
    void operator()(T* p) noexcept {
        if (p)
            alloc.deallocate(p, 1);
    }
private:
    [[no_unique_address]] A alloc; //!< The stored allocator
};

//! An allocater-aware replacement of \c std::unique_ptr
/*! Instances are usually created by allocate_unique().
 * \tparam T the allocated type
 * \tparam A an allocator template
 * \tparam U the type allocated by \a A
 * \test in file test_allocated.cpp */
template <class T, impl::allocator A>
using unique_ptr_alloc = std::unique_ptr<T, deleter<T, A>>;

//! A function to allocate an object and create unique_ptr_alloc.
/*! \tparam T the allocated type
 * \tparam A an allocator template
 * \tparam U the type allocated by \a A
 * \tparam Args constructor parameters of \a T
 * \param[in] alloc an allocator to use for allocation and deallocation
 * \param[in] args arguments for a constructor of T
 * \return a unique_ptr_alloc pointing to the allocated object
 * \test in file test_allocated.cpp */
template <class T, impl::allocator A, class ...Args>
auto allocate_unique(const A& alloc, Args&& ...args)
{
    using a_t =
        typename std::allocator_traits<A>::template rebind_alloc<T>;
    a_t a{alloc};
    T* p = new(a.allocate(1)) T(std::forward<Args>(args)...);
    return
        unique_ptr_alloc<T, a_t>(p, deleter<T, a_t>(std::move(alloc)));
}

//! An instance of template \c std::basic_string using an allocator
/*! \tparam A an allocator type */
template <impl::allocator A>
using a_basic_string = std::basic_string<char, std::char_traits<char>,
    typename std::allocator_traits<A>::template rebind_alloc<char>>;

//! A hash function for a_basic_string
/*! It is needed, because the standard library provides a specialization of
 * \c std::hash for \c std::string, but not for instances of template \c
 * std::basic_string that use a custom allocator.
 * \tparam A an allocator type */
template <impl::allocator A> struct a_basic_string_hash {
    //! Computes a hash value.
    /*! The result is the same as for the corresponding \c std::string, because
     * the C++ standard requires the same values of \c std::hash for
     * \c std::string and \c std::string_view.
     * \param[in] s a value to be hashed
     * \return the hash value */
    size_t operator()(const a_basic_string<A>& s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
};

//! An instance of template \c std::vector using an allocator
/*! \tparam T a type of vector elements
 * \tparam A an allocator type */
template <class T, impl::allocator A>
using a_basic_vector = std::vector<T,
    typename std::allocator_traits<A>::template rebind_alloc<T>>;

//! An instance of template \c std::deque using an allocator
/*! \tparam T a type of deque elements
 * \tparam A an allocator type */
template <class T, impl::allocator A>
using a_basic_deque = std::deque<T,
    typename std::allocator_traits<A>::template rebind_alloc<T>>;

namespace impl {

//! Checks if \a T is an instance of template a_basic_string.
/*! \tparam T a type */
template <class T> concept is_a_basic_string = requires {
    typename T::allocator_type;
    requires std::is_same_v<T, a_basic_string<typename T::allocator_type>>;
};

//! Selects a hash function implementation for type \a T.
/*! \tparam T a type
 * \tparam B used to detect a_basic_string */
template <class T, bool B = is_a_basic_string<T>> struct hash_for {
    //! The hash function implementation for type \a T
    using type = std::hash<T>;
};

//! A specialization for a_basic_string
/*! \tparam T an instance of a_basic_string */
template <class T> struct hash_for<T, true> {
    //! The hash function implementation for a_basic_string
    using type = a_basic_string_hash<typename T::allocator_type>;
};

//! An alias for \c hash_for<T>::type
/*! \tparam T a type */
template <class T> using hash_for_t = typename hash_for<T>::type;

} // namespace impl

//! An instance of template \c std::unordered_map using an allocator
/*! \tparam K a key type
 * \tparam T a value type
 * \tparam A an allocator type */
template <class K, class T, impl::allocator A>
using a_basic_hash = std::unordered_map<K, T, impl::hash_for_t<K>,
    std::equal_to<K>,
    typename std::allocator_traits<A>::template rebind_alloc<
        std::pair<const K, T>>>;

//! Automatic reducing of capacity of std container and string classes
/*! This function shrinks the capacity if there is a large unused capacity.
 * Common implementations double the capacity when reallocating, therefore we
 * reduce the capacity when only 1/3 of it is used. This prevents frequent
 * reallocations if the size oscillates around the reallocation boundary.
 * Special cases:
 * \arg \c std::hash -- it manipulates the load factor instead of the capacity
 * \arg \c std::deque -- it does nothing, because \c std::deque handles freeing
 * unsused memory automatically
 * \tparam a std container or string type
 * \param[in] v a container or a string */
template <class T> void std_container_shrink(T& v) {
    if constexpr (requires (T v) { v.size(); v.capacity(); v.shrink_to_fit(); })
    {
        if (v.size() <= v.capacity() / 3)
            v.shrink_to_fit();
    } else if constexpr(requires (T v) { v.load_factor(); }) {
        if (v.load_factor() <= v.max_load_factor() / 3)
            v.rehash(v.size() / v.max_load_factor() / 2 * 3);
    } else {
        // assume automatic shrinking implemented by T
    }
}

} // namespace threadscript
