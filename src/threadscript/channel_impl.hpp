#pragma once

/*! \file
 * \brief The implementation part of channel.hpp
 */

#include "threadscript/channel.hpp"

namespace threadscript {

template <impl::allocator A>
basic_channel<A>::basic_channel(
        typename basic_channel<A>::tag t,
        std::shared_ptr<const typename basic_channel<A>::method_table>
            methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node):
    impl::basic_channel_base<A>(t, methods, thread, l_vars, node)
{
    this->set_mt_safe();
}

} // namespace threadscript
