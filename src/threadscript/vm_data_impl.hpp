#pragma once

/*! \file
 * \brief The implementation part of vm_data.hpp
 */

#include "threadscript/vm_data.hpp"

namespace threadscript {

/*** basic_value ************************************************************/

template <class Allocator> void basic_value<Allocator>::set_mt_safe()
{
    _mt_safe = true;
}

/*** basic_typed_value ******************************************************/

template <class Derived, class T, const char* Name, impl::allocator Allocator>
template <class A> requires impl::uses_allocator<T, A>
basic_typed_value<Derived, T, Name, Allocator>::basic_typed_value(tag,
                                                                  const A& a):
    T{a}
{
}

template <class Derived, class T, const char* Name, class Allocator>
template <class A> requires !impl::uses_allocator<T, A>
basic_typed_value<Derived, T, Name, Allocator>::basic_typed_value(tag,
                                                                  const A&):
    T{}
{
}

template <class Derived, class T, const char* Name, class Allocator>
value_ptr basic_typed_value<Derived, T, Name, Allocator>::shallow_copy_impl(
                                                        const Allocator& alloc)
    const override
{
    auto p = std::make_shared<Derived>(alloc);
    p->value() = value();
    return p;
}

template <class Derived, class T, const char* Name, class Allocator>
std::string_view
basic_typed_value<Derived, T, Name, Allocator::type_name() const noexcept
{
    return Name;
}

} // namespace threadscript
