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
basic_typed_value<Derived, T, Name, Allocator>::basic_typed_value(tag,
                                                      const Allocator& alloc):
    basic_typed_value(tag2{}, alloc)
{
}

//! \cond
template <class Derived, class T, const char* Name, impl::allocator Allocator>
template <class A> requires impl::uses_allocator<T, A>
basic_typed_value<Derived, T, Name, Allocator>::basic_typed_value(tag2,
                                                                  const A& a):
    data{a}
{
}

template <class Derived, class T, const char* Name, impl::allocator Allocator>
template <class A> requires (!impl::uses_allocator<T, A>)
basic_typed_value<Derived, T, Name, Allocator>::basic_typed_value(tag2,
                                                                  const A&):
    data{}
{
}
//! \endcond

template <class Derived, class T, const char* Name, impl::allocator Allocator>
auto basic_typed_value<Derived, T, Name, Allocator>::shallow_copy_impl(
                                                    const Allocator& alloc,
                                                    std::optional<bool> mt_safe)
    const -> typename basic_value<Allocator>::value_ptr
{
    auto p = create(alloc);
    p->value() = value();
    if (mt_safe.value_or(this->mt_safe()))
        p->set_mt_safe();
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

/*** basic_value_array *******************************************************/

template <impl::allocator Allocator>
void basic_value_array<Allocator>::set_mt_safe()
{
    for (auto&& v: this->cvalue())
        if (v && !v->mt_safe())
            throw exception::value_mt_unsafe();
    return impl::basic_value_array_base<Allocator>::set_mt_safe();
}

/*** basic_value_hash ********************************************************/

template <impl::allocator Allocator>
void basic_value_hash<Allocator>::set_mt_safe()
{
    for (auto&& v: this->cvalue())
        if (v.second && !v.second->mt_safe())
            throw exception::value_mt_unsafe();
    return impl::basic_value_hash_base<Allocator>::set_mt_safe();
}

} // namespace threadscript
