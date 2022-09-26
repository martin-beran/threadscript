#pragma once

/*! \file
 * \brief The implementation part of shared_vector.hpp
 */

#include "threadscript/shared_vector.hpp"

namespace threadscript {

template <impl::allocator A>
basic_shared_vector<A>::basic_shared_vector(
        typename basic_shared_vector<A>::tag t,
        std::shared_ptr<const typename basic_shared_vector<A>::method_table>
            methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node):
    impl::basic_shared_vector_base<A>(t, methods, thread, l_vars, node)
{
    this->set_mt_safe();
}

template <impl::allocator A> basic_shared_vector<A>::value_ptr
basic_shared_vector<A>::at(typename threadscript::basic_state<A>& thread,
    typename threadscript::basic_symbol_table<A>& l_vars,
    const typename threadscript::basic_code_node<A>& node)
{
    size_t narg = this->narg(node);
    if (narg !=2 && narg != 3)
        throw exception::op_narg();
    size_t i = this->arg_index(thread, l_vars, node, 1);
    if (narg == 2) {
        std::lock_guard lck(mtx);
        if (i >= data.size())
            throw exception::value_out_of_range();
        return data[i];
    } else {
        assert(narg == 3);
        auto v = this->arg(thread, l_vars, node, 2);
        std::lock_guard lck(mtx);
        if (i >= data.max_size())
            throw exception::value_out_of_range();
        if (i >= data.size())
            data.resize(i + 1);
        return data[i] = v;
    }
}

template <impl::allocator A> basic_shared_vector<A>::value_ptr
basic_shared_vector<A>::erase(
    typename threadscript::basic_state<A>& thread,
    typename threadscript::basic_symbol_table<A>& l_vars,
    const typename threadscript::basic_code_node<A>& node)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    if (narg == 1) {
        std::lock_guard lck(mtx);
        data.clear();
    } else {
        size_t i = this->arg_index(thread, l_vars, node, 1);
        std::lock_guard lck(mtx);
        if (i < data.size()) {
            data.resize(i);
            std_container_shrink(data);
        }
    }
    return nullptr;
}

template <impl::allocator A> basic_shared_vector<A>::value_ptr
basic_shared_vector<A>::size(typename threadscript::basic_state<A>& thread,
    typename threadscript::basic_symbol_table<A>&,
    const typename threadscript::basic_code_node<A>& node)
{
    if (this->narg(node) != 1)
        throw exception::op_narg();
    auto res = value_unsigned::create(thread.get_allocator());
    std::lock_guard lck(mtx);
    res->value() = data.size();
    return res;
}

template <impl::allocator A> basic_shared_vector<A>::method_table
basic_shared_vector<A>::init_methods()
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
