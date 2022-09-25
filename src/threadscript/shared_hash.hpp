#pragma once

/*! \file
 * \brief A hash class that can be modified by multiple threads.
 */

#include "threadscript/vm_data.hpp"

namespace threadscript {

//! A thread-safe hash class
/*! Unlike basic_value_hash, all objects of this class are marked thread-safe
 * and can be accessed and modified by multiple threads simultaneously.
 * \tparam A an allocator type
 * \threadsafe{safe,safe}
 * \test in file test_shared_hash.cpp */
template <impl::allocator A>
class basic_shared_hash:
    public basic_value_object<basic_shared_hash<A>, "shared_hash", A>
{
    
};

} // namespace threadscript
