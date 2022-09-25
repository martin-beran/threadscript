#pragma once

/*! \file
 * \brief A channel for communicating among threads.
 */

#include "threadscript/vm_data.hpp"

namespace threadscript {

//! A thread-safe communication channel class
/*! This class provides a means to pass values among threads and to synchronize
 * threads.
 * \tparam A an allocator type
 * \threadsafe{safe,safe}
 * \test in file test_channel.cpp */
template <impl::allocator A>
class basic_channel:
    public basic_value_object<basic_channel<A>, "channel", A>
{
    
};

} // namespace threadscript
