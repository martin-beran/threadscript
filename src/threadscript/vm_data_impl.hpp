#pragma once

/*! \file
 * \brief The implementation part of vm_data.hpp
 */

#include "threadscript/vm_data.hpp"

namespace threadscript {

/*** basic_value ************************************************************/

template <impl::allocator Allocator>
void basic_value<Allocator>::set_mt_safe()
{
    _mt_safe = true;
}

/*** basic_typed_value ******************************************************/

template <class Derived, class T, const char* Name, impl::allocator Allocator>
template <class A> requires impl::uses_allocator<T, A>
basic_typed_value<Derived, T, Name, Allocator>::basic_typed_value(const A& a):
    data{a}
{
}

template <class Derived, class T, const char* Name, impl::allocator Allocator>
template <class A> requires (!impl::uses_allocator<T, A>)
basic_typed_value<Derived, T, Name, Allocator>::basic_typed_value(const A&):
    data{}
{
}

template <class Derived, class T, const char* Name, impl::allocator Allocator>
auto basic_typed_value<Derived, T, Name, Allocator>::shallow_copy_impl(
                                                    const Allocator& alloc)
    const -> typename basic_value<Allocator>::value_ptr
{
    auto p = std::make_shared<Derived>(tag{}, alloc);
    p->value() = value();
    return p;
}

template <class Derived, class T, const char* Name, impl::allocator Allocator>
std::string_view
basic_typed_value<Derived, T, Name, Allocator>::type_name() const noexcept
{
    // It is here and not in the declaration of class basic_typed_value,
    // because Derived must be a complete type here
    static_assert(std::is_final_v<Derived>);
    return Name;
}

} // namespace threadscript
