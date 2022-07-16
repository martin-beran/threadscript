#pragma once

/*! \file
 * \brief Various concepts used in ThreadScript sources
 */

#include <memory>
#include <type_traits>

namespace threadscript::impl {

//! Basic requirements for a type used as an allocator.
/*! \tparam A an allocator type */
template <class A>
concept allocator = requires (A a, typename A::value_type* p) {
    typename A::value_type;
    A(a);
    a.allocate(size_t{});
    a.deallocate(p, size_t{});
};

//! Checks that type T takes an allocator as a constructor parameter
/*! \tparam T a type
 * \tparam A an allocator type */
template <class T, class A>
concept uses_allocator = requires {
    requires allocator<A>;
    typename T::allocator_type;
};

//! Requirements for a template argument of threadscript::finally
template <class F>
concept finally_fun = std::is_nothrow_invocable_v<F>;

} // namespace threadscript::impl
