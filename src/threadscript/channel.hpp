#pragma once

/*! \file
 * \brief A channel for communicating among threads.
 */

#include "threadscript/vm_data.hpp"

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
 * threads.
 * \tparam A an allocator type
 * \threadsafe{safe,safe}
 * \test in file test_channel.cpp */
template <impl::allocator A>
class basic_channel: public impl::basic_channel_base<A> {
public:
    //! It marks the object mt-safe.
    /*! \copydetails basic_value_object<A>::basic_value_object() */
    basic_channel(typename basic_channel::tag t,
        std::shared_ptr<const typename basic_channel::method_table> methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node);
};

} // namespace threadscript
