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
    static_assert(std::is_same_v<stack_t, a_basic_deque<stack_frame, A>>);
    // freeing unused space of std::deque is done automatically and there is no
    // std::deque::capacity(). Uncomment the following two lines if the stack
    // implementation is changed from std::deque to std::vector.
    // if (stack.size() <= stack.capacity() / 3)
    //    stack.shrink_to_fit();
}

template <impl::allocator A>
auto basic_state<A>::push_frame(stack_frame&& frame) -> stack_frame&
{
    if (stack.size() >= max_stack)
        throw exception::op_recursion(current_stack());
    stack.push_back(std::move(frame));
    return stack.back();
}

} // namespace threadscript
