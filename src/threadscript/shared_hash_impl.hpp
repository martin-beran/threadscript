#pragma once

/*! \file
 * \brief The implementation part of shared_hash.hpp
 */

#include "threadscript/shared_hash.hpp"

namespace threadscript {

template <impl::allocator A>
basic_shared_hash<A>::basic_shared_hash(
        typename basic_shared_hash<A>::tag t,
        std::shared_ptr<const typename basic_shared_hash<A>::method_table>
            methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node):
    impl::basic_shared_hash_base<A>(t, methods, thread, l_vars, node)
{
    this->set_mt_safe();
}

} // namespace threadscript
