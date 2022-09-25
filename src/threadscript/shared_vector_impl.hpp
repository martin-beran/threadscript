#pragma once

/*! \file
 * \brief The implementation part of shared_vector.hpp
 */

#include "threadscript/shared_vector.hpp"

namespace threadscript {

template <impl::allocator A>
basic_shared_vector::basic_shared_vector(tag t,
                                 std::shared_ptr<const method_table> methods,
                                 ts::state& thread, ts::symbol_table& l_vars,
                                 const ts::code_node& node):
    basic_value_object(t, methods, thread, l_vars, node)
{
    set_mt_safe();
}

template <impl::allocator A>
auto basic_shared_vector<A>::at(ts::state& /*thread*/, ts::symbol_table& /*l_vars*/,
                                const ts::code_node& /*node*/) -> value_ptr
{
    // TODO
    return nullptr;
}

template <impl::allocator A>
auto basic_shared_vector<A>::erase(ts::state& /*thread*/, ts::symbol_table& /*l_vars*/,
                                   const ts::code_node& /*node*/) -> value_ptr
{
    // TODO
    return nullptr;
}

template <impl::allocator A>
auto basic_shared_vector<A>::size(ts::state& thread, ts::symbol_table&,
                                  const ts::code_node& node) -> value_ptr
{
    if (narg(node) != 1)
        throw exception::op_narg();
    auto res = ts::value_unsigned::create(thread.get_allocator());
    std::lock_guard lck(mtx);
    res->value() = data.size();
    return res;
}

template <impl::allocator A> method_table basic_shared_vector<A>::init_methods()
{
    return {
        //! [methods]
        {"at", &basic_shared_vector::at},
        {"erase", &basic_shared_vector::erase},
        {"size", &basic_shared_vector::size},
        //! [methods]
    };
}

} // namespace threadscript
