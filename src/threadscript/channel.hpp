#pragma once

/*! \file
 * \brief A channel for communicating among threads.
 */

#include "threadscript/vm_data.hpp"

#include <condition_variable>

namespace threadscript {

template <impl::allocator A> class basic_channel;

namespace impl {
//! The name of channel
inline constexpr char name_channel[] = "channel";
//! The base class of basic_channel
/*! \tparam A an allocator type */
template <allocator A> using basic_channel_base =
    basic_value_object<basic_channel<A>, name_channel, A>;
} // namespace impl

//! A thread-safe communication channel class
/*! This class provides a means to pass values among threads and to synchronize
 * threads. It is a FIFO queue of messages with fixed capacity, set upon
 * channel creation. Any thread can post a message to the queue by methods
 * send() and try_send(), or retrieve a message by methods recv() and
 * try_recv(). If posting a message and the channel is full (the number of
 * stored messages equals the capacity), send() blocks, while try_send() throws
 * exception::op_would_block. If retrieving a message and the channel is empty,
 * recv() blocks, while try_recv() throws exception::op_would_block.
 *
 * If the capacity is zero, post and retrieve operations must occur
 * simultaneously in order to proceed successfully. send() blocks until recv()
 * or try_recv() is called. try_send() succeeds only if there is recv()
 * waiting. recv() blocks until send() or try_recv() is called. try_recv()
 * succeeds only if there is send() waiting. A pair of try_send() and
 * try_recv() never succeeds for a zero capacity channel.
 *
 * Methods:
 * \snippet channel_impl.hpp methods
 * \tparam A an allocator type
 * \threadsafe{safe,safe}
 * \test in file test_channel.cpp */
template <impl::allocator A>
class basic_channel final: public impl::basic_channel_base<A> {
public:
    //! Creates the channel object.
    /*! It marks the object mt-safe.
     * \param[in] t an ignored parameter that prevents using this
     * \param[in] methods the mapping from method names to implementations
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with constructor arguments:
     *     \arg \c capacity the channel capacity, of type \c int or \c unsigned
     * \throw exception::op_narg if the number of arguments is not 1
     * \throw exception::value_null if \a capacity is \c null
     * \throw exception::value_type if \a capacity does not have type \c int or
     * \c c unsigned
     * \throw exception::value_out_of_range if \a capacity is negative */
    basic_channel(typename basic_channel::tag t,
        std::shared_ptr<const typename basic_channel::method_table> methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node);
    //! \copydoc basic_value_object::init_methods()
    [[nodiscard]] static
    typename basic_channel::method_table init_methods();
private:
    //! Sends a message to the channel in the blocking mode.
    /*! A message can be any thread-safe value or \c null. If the channel is
     * not full, the message is appended at the end of the message queue.
     * Otherwise, the operation blocks until a message is received.
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     *     \arg \c value the value sent to the channel
     * \return \c null
     * \throw exception::op_narg if the number of arguments (incl. \c
     * method_name) is not 2
     * \throw exception::value_mt_unsafe if \a value is not mt-safe */
    typename basic_channel::value_ptr
    send(typename threadscript::basic_state<A>& thread,
         typename threadscript::basic_symbol_table<A>& l_vars,
         const typename threadscript::basic_code_node<A>& node);
    //! Sends a message to the channel in the nonblocking mode.
    /*! A message can be any thread-safe value or \c null. If the channel is
     * not full, the message is appended at the end of the message queue.
     * Otherwise, the operation throws exception::op_would_block.
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     *     \arg \c value the value sent to the channel
     * \return \c null
     * \throw exception::op_narg if the number of arguments (incl. \c
     * method_name) is not 2
     * \throw exception::value_mt_unsafe if \a value is not mt-safe
     * \throw exception::op_would_block if a send() called instead of this
     * method would block */
    typename basic_channel::value_ptr
    try_send(typename threadscript::basic_state<A>& thread,
             typename threadscript::basic_symbol_table<A>& l_vars,
             const typename threadscript::basic_code_node<A>& node);
    //! Receives a message from the channel in the blocking mode.
    /*! If the channel is not empty, the first message in the message queue is
     * removed and returned. Otherwise, the operation blocks until a message
     * is sent.
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     * \return the received value (can be \c null)
     * \throw exception::op_narg if the number of arguments (incl. \c
     * method_name) is not 1 */
    typename basic_channel::value_ptr
    recv(typename threadscript::basic_state<A>& thread,
         typename threadscript::basic_symbol_table<A>& l_vars,
         const typename threadscript::basic_code_node<A>& node);
    //! Receives a message from the channel in the nonblocking mode.
    /*! If the channel is not empty, the first message in the message queue is
     * removed and returned. Otherwise, the operation throws
     * exception::op_would_block.
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     * \return the received value (can be \c null)
     * \throw exception::op_narg if the number of arguments (incl. \c
     * method_name) is not 1
     * \throw exception::op_would_block if a recv() called instead of this
     * method would block */
    typename basic_channel::value_ptr
    try_recv(typename threadscript::basic_state<A>& thread,
             typename threadscript::basic_symbol_table<A>& l_vars,
             const typename threadscript::basic_code_node<A>& node);
    //! Gets the number of waiting senders/receivers.
    /*! \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     * \return a value of type \c int; the number of threads blocked in send()
     * if positive; minus the number of threads blocked in recv() if negative
     * \throw exception::op_narg if the number of arguments (incl. \c
     * method_name) is not 1 */
    typename basic_channel::value_ptr
    balance(typename threadscript::basic_state<A>& thread,
            typename threadscript::basic_symbol_table<A>& l_vars,
            const typename threadscript::basic_code_node<A>& node);
    //! Checks if the queue has space for at least one value.
    /*! \return \c true if \ref data is not full, \c false if it is full */
    bool has_space() {
        return it_push != it_pop;
    }
    //! Checks if the queue is nonempty.
    /*! \return \c if \ref data is not empty, \c false if it is empty */
    bool has_data() {
        return it_pop != data.end();
    }
    //! Pushes a value to the end of the queue.
    /*! It assumes the queue is not full.
     * \param[in] val the value to be pushed */
    void push(typename basic_channel::value_ptr&& val);
    //! Pops a value from the front of the queue.
    /*! It assumes the queue is not empty.
     * \return val the popped value */
    typename basic_channel::value_ptr pop();
    //! Gets the capacity of the queue.
    /*! \return the capacity, as initialized by the constructor */
    size_t capacity() {
        return data.size();
    }
    //! The message queue (for nonzero \ref capacity)
    a_basic_vector<typename basic_channel::value_ptr, A> data;
    //! The position for push() (for nonzero \ref capacity)
    decltype(data)::iterator it_push{};
    //! The position for pop() (for nonzero \ref capacity)
    decltype(data)::iterator it_pop{};
    //! A value storage (for zero \ref capacity)
    std::optional<typename basic_channel::value_ptr> value;
    //! The mutex for synchronizing access from threads
    std::mutex mtx;
    //! Used to wait in send()
    std::condition_variable cond_send;
    //! Used to wait in recv()
    std::condition_variable cond_recv;
    //! Tracks waiting senders
    intmax_t senders = 0;
    //! Tracks waiting receivers
    intmax_t receivers = 0;
};

} // namespace threadscript
