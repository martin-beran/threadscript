#pragma once

/*! \file
 * \brief The implementation part of symbol_table.hpp
 */

#include "threadscript/symbol_table.hpp"

namespace threadscript {

/*** basic_symbol_table ******************************************************/

template <impl::allocator A>
bool basic_symbol_table<A>::contains(const key_type& name,
                                     bool use_parent) const
{
    if (symbols().contains(name))
        return true;
    if (use_parent)
        if (auto pt = parent_table())
            return pt->contains(name);
    return false;
}

template <impl::allocator A> template <class K>
bool basic_symbol_table<A>::contains(const K& name,
                                     bool use_parent) const
{
    if (symbols().contains(name))
        return true;
    if (use_parent)
        if (auto pt = parent_table())
            return pt->contains(name);
    return false;
}

template <impl::allocator A>
bool basic_symbol_table<A>::erase(const key_type& name)
{
    return symbols().erase(name) > 0;
}

template <impl::allocator A> template <class K>
bool basic_symbol_table<A>::erase(const K& name)
{
    return symbols().erase(name) > 0;
}

template <impl::allocator A>
bool basic_symbol_table<A>::insert(key_type name, value_type value)
{
    return symbols().insert_or_assign(std::move(name), std::move(value)).second;
}

template <impl::allocator A> template <class K>
bool basic_symbol_table<A>::insert(const K& name, value_type value)
{
    return symbols().insert_or_assign(key_type(name, get_allocator()),
                                      std::move(value)).second;
}

template <impl::allocator A>
auto basic_symbol_table<A>::lookup(const key_type& name, bool use_parent) const
    -> std::optional<value_type>
{
    if (auto it = symbols().find(name); it != data.end())
        return it->second;
    if (use_parent)
        if (auto pt = parent_table())
            return pt->lookup(name);
    return std::nullopt;
}

template <impl::allocator A> template <class K>
auto basic_symbol_table<A>::lookup(const K& name, bool use_parent) const
    -> std::optional<value_type>
{
    if (auto it = symbols().find(name); it != data.end())
        return it->second;
    if (use_parent)
        if (auto pt = parent_table())
            return pt->lookup(name);
    return std::nullopt;
}

} // namespace threadscript
