#pragma once

/*! \file
 * \brief The implementation part of vm_data.hpp
 */

#include "threadscript/vm_data.hpp"
#include "threadscript/code.hpp"

namespace threadscript {

/*** basic_value ************************************************************/

template <impl::allocator A>
auto basic_value<A>::eval(basic_state<A>&, const basic_symbol_table<A>&,
    const std::vector<std::reference_wrapper<basic_symbol_table<A>>>&,
    const basic_code_node<A>&, std::string_view) -> value_ptr
{
    return this->shared_from_this();
}

template <impl::allocator A>
void basic_value<A>::set_mt_safe()
{
    _mt_safe = true;
}

template <impl::allocator A> std::ostream&
operator<<(std::ostream& os, const typename basic_value<A>::value_ptr& p)
{
    if (p)
        p->write(os);
    else
        os << "null";
    return os;
}

/*** basic_typed_value ******************************************************/

template <class Derived, class T, str_literal Name, impl::allocator A>
basic_typed_value<Derived, T, Name, A>::basic_typed_value(tag, const A& alloc):
    basic_typed_value(tag2{}, alloc)
{
}

//! \cond
template <class Derived, class T, str_literal Name, impl::allocator A>
template <class Alloc> requires impl::uses_allocator<T, Alloc>
basic_typed_value<Derived, T, Name, A>::basic_typed_value(tag2, const Alloc& a):
    data{a}
{
}

template <class Derived, class T, str_literal Name, impl::allocator A>
template <class Alloc> requires (!impl::uses_allocator<T, Alloc>)
basic_typed_value<Derived, T, Name, A>::basic_typed_value(tag2, const Alloc&):
    data{}
{
}
//! \endcond

template <class Derived, class T, str_literal Name, impl::allocator A>
auto basic_typed_value<Derived, T, Name, A>::shallow_copy_impl(const A& alloc,
                                                    std::optional<bool> mt_safe)
    const -> typename basic_value<A>::value_ptr
{
    auto p = create(alloc);
    p->value() = value();
    if (mt_safe.value_or(this->mt_safe()))
        p->set_mt_safe();
    return p;
}

template <class Derived, class T, str_literal Name, impl::allocator A>
std::string_view basic_typed_value<Derived, T, Name, A>::type_name()
    const noexcept
{
    return Name;
}

/*** basic_value_bool ********************************************************/

template <impl::allocator A>
void basic_value_bool<A>::write(std::ostream& os) const
{
    if (this->cvalue())
        os << "true";
    else
        os << "false";
}

/*** basic_value_int *********************************************************/

template <impl::allocator A>
void basic_value_int<A>::write(std::ostream& os) const
{
    os << this->cvalue();
}

/*** basic_value_unsigned ****************************************************/

template <impl::allocator A>
void basic_value_unsigned<A>::write(std::ostream& os) const
{
    os << this->cvalue();
}

/*** basic_value_string ******************************************************/

template <impl::allocator A>
void basic_value_string<A>::write(std::ostream& os) const
{
    os << this->cvalue();
}

/*** basic_value_array *******************************************************/

template <impl::allocator A> void basic_value_array<A>::set_mt_safe()
{
    for (auto&& v: this->cvalue())
        if (v && !v->mt_safe())
            throw exception::value_mt_unsafe();
    return impl::basic_value_array_base<A>::set_mt_safe(); }

/*** basic_value_hash ********************************************************/

template <impl::allocator A> void basic_value_hash<A>::set_mt_safe()
{
    for (auto&& v: this->cvalue())
        if (v.second && !v.second->mt_safe())
            throw exception::value_mt_unsafe();
    return impl::basic_value_hash_base<A>::set_mt_safe();
}

} // namespace threadscript
