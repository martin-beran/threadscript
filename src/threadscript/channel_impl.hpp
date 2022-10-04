#pragma once

/*! \file
 * \brief The implementation part of channel.hpp
 */

#include "threadscript/channel.hpp"

namespace threadscript {

template <impl::allocator A>
basic_channel<A>::basic_channel(
        typename basic_channel<A>::tag,
        std::shared_ptr<const typename basic_channel<A>::method_table>
            methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node):
    impl::basic_channel_base<A>(typename basic_channel::tag_args{}, methods,
                                thread, l_vars, node)
{
    size_t narg = this->narg(node);
    if (narg != 1)
        throw exception::op_narg();
    auto c = this->arg_index(thread, l_vars, node, 0);
    if (c != 0) {
        data.resize(c);
        it_push = data.begin();
        it_pop = data.end();
    }
    this->set_mt_safe();
}

template <impl::allocator A> basic_channel<A>::value_ptr
basic_channel<A>::balance(
    typename threadscript::basic_state<A>& thread,
    typename threadscript::basic_symbol_table<A>&,
    const typename threadscript::basic_code_node<A>& node)
{
    size_t narg = this->narg(node);
    if (narg != 1)
        throw exception::op_narg();
    std::unique_lock lck{mtx};
    auto result = basic_value_int<A>::create(thread.get_allocator());
    result->value() = config::value_int_type(senders - receivers);
    return result;
}

template <impl::allocator A> basic_channel<A>::method_table
basic_channel<A>::init_methods()
{
    return {
        //! [methods]
        {"balance", &basic_channel::balance},
        {"recv", &basic_channel::recv},
        {"send", &basic_channel::send},
        {"try_recv", &basic_channel::try_recv},
        {"try_send", &basic_channel::try_send},
        //! [methods]
    };
}

template <impl::allocator A> typename basic_channel<A>::value_ptr
basic_channel<A>::pop()
{
    auto result = std::move(*it_pop++);
    if (it_pop == data.end())
        it_pop = data.begin();
    if (it_pop == it_push)
        it_pop = data.end();
    return result;
}

template <impl::allocator A> void
basic_channel<A>::push(typename basic_channel::value_ptr&& val)
{
    if (it_pop == data.end())
        it_pop = it_push;
    *it_push++ = std::move(val);
    if (it_push == data.end())
        it_push = data.begin();
}

template <impl::allocator A> basic_channel<A>::value_ptr
basic_channel<A>::recv(
    typename threadscript::basic_state<A>&,
    typename threadscript::basic_symbol_table<A>&,
    const typename threadscript::basic_code_node<A>& node)
{
    return recv_impl(node,
        [&](auto& lck) {
            ++receivers;
            cond_recv.wait(lck, [&]() { return senders > 0 && value; });
        },
        [&](auto& lck) {
            ++receivers;
            cond_recv.wait(lck, [&]() { return has_data(); });
            --receivers;
        });
}

template <impl::allocator A> template <class F0, class F>
basic_channel<A>::value_ptr basic_channel<A>::recv_impl(
    const typename threadscript::basic_code_node<A>& node,
    F0&& cond0, F&& cond)
{
    size_t narg = this->narg(node);
    if (narg != 1)
        throw exception::op_narg();
    std::unique_lock lck{mtx};
    if (capacity() == 0) {
        cond0(lck);
        auto result = std::move(*value);
        value.reset();
        --senders;
        lck.unlock();
        cond_send.notify_one();
        return result;
    } else {
        cond(lck);
        auto result = pop();
        lck.unlock();
        cond_send.notify_one();
        return result;
    }
}

template <impl::allocator A> basic_channel<A>::value_ptr
basic_channel<A>::send(
    typename threadscript::basic_state<A>& thread,
    typename threadscript::basic_symbol_table<A>& l_vars,
    const typename threadscript::basic_code_node<A>& node)
{
    return send_impl(thread, l_vars, node,
        [&](auto& lck) {
            ++senders;
            cond_send.wait(lck, [&]() { return receivers > 0 && !value; });
        },
        [&](auto& lck) {
            ++senders;
            cond_send.wait(lck, [&]() { return has_space(); });
            --senders;
        });
}

template <impl::allocator A> template <class F0, class F>
basic_channel<A>::value_ptr basic_channel<A>::send_impl(
    typename threadscript::basic_state<A>& thread,
    typename threadscript::basic_symbol_table<A>& l_vars,
    const typename threadscript::basic_code_node<A>& node,
    F0&& cond0, F&& cond)
{
    size_t narg = this->narg(node);
    if (narg != 2)
        throw exception::op_narg();
    auto v = this->arg(thread, l_vars, node, 1);
    if (v && !v->mt_safe())
        throw exception::value_mt_unsafe();
    std::unique_lock lck{mtx};
    if (capacity() == 0) {
        cond0(lck);
        value = std::move(v);
        --receivers;
        lck.unlock();
        cond_recv.notify_one();
    } else {
        cond(lck);
        push(std::move(v));
        lck.unlock();
        cond_recv.notify_one();
    }
    return nullptr;
}

template <impl::allocator A> basic_channel<A>::value_ptr
basic_channel<A>::try_recv(
    typename threadscript::basic_state<A>&,
    typename threadscript::basic_symbol_table<A>&,
    const typename threadscript::basic_code_node<A>& node)
{
    return recv_impl(node,
        [&](auto&) {
            if (senders == 0 || !value)
                throw exception::op_would_block();
            ++receivers;
        },
        [&](auto&) {
            if (!has_data())
                throw exception::op_would_block();
        });
}

template <impl::allocator A> basic_channel<A>::value_ptr
basic_channel<A>::try_send(
    typename threadscript::basic_state<A>& thread,
    typename threadscript::basic_symbol_table<A>& l_vars,
    const typename threadscript::basic_code_node<A>& node)
{
    return send_impl(thread, l_vars, node,
        [&](auto&) {
            if (receivers == 0 || value)
                throw exception::op_would_block();
            ++senders;
        },
        [&](auto&) {
            if (!has_space())
                throw exception::op_would_block();
        });
}

} // namespace threadscript
