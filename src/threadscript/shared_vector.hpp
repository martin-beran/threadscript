#pragma once

/*! \file
 * \brief A vector class that can be modified by multiple threads.
 */

#include "threadscript/vm_data.hpp"

namespace threadscript {

//! A thread-safe vector class
/*! Unlike basic_value_vector, all objects of this class are marked thread-safe
 * and can be accessed and modified by multiple threads simultaneously.
 * \tparam A an allocator type
 * \threadsafe{safe,safe}
 * \test in file test_shared_vector.cpp */
template <impl::allocator A>
class basic_shared_vector:
    public basic_value_object<basic_shared_vector<A>, "shared_vector", A>
{
    
};

} // namespace threadscript
