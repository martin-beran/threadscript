#pragma once

/*! \file
 * \brief The implementation part of virtual_machine.hpp
 */

#include "threadscript/virtual_machine.hpp"

namespace threadscript {

/*** basic_state *************************************************************/

template <impl::allocator A>
auto basic_state<A>::current_stack() const noexcept -> stack_trace
{
    try {
        stack_trace trace;
        trace.reserve(stack.size());
        for (auto it = stack.rbegin(); it != stack.rend(); ++it)
            trace.push_back(it->location);
        return trace;
    } catch (...) {
        return stack_trace{};
    }
}

template <impl::allocator A>
void basic_state<A>::pop_frame() noexcept
{
    assert(!stack.empty());
    stack.pop_back();
    std_container_shrink(stack);
}

template <impl::allocator A>
auto basic_state<A>::push_frame(stack_frame&& frame) -> stack_frame&
{
    if (stack.size() >= max_stack)
        throw exception::op_recursion(current_stack());
    stack.push_back(std::move(frame));
    return stack.back();
}

template <impl::allocator A>
void basic_state<A>::update_sh_vars()
{
    sh_vars = vm.sh_vars.load();
    t_vars.parent = sh_vars.get();
}

} // namespace threadscript
