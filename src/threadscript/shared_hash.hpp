#pragma once

/*! \file
 * \brief A hash class that can be modified by multiple threads.
 */

#include "threadscript/vm_data.hpp"

namespace threadscript {

template <impl::allocator A> class basic_shared_hash;

namespace impl {
//! The name of shared_hash
inline constexpr char name_shared_hash[] = "shared_hash";
//! The base class of basic_shared_hash
/*! \tparam A an allocator type */
template <allocator A> using basic_shared_hash_base =
    basic_value_object<basic_shared_hash<A>, name_shared_hash, A>;
} // namespace impl

//! A thread-safe hash class
/*! Unlike basic_value_hash, all objects of this class are marked thread-safe
 * and can be accessed and modified by multiple threads simultaneously.
 * \tparam A an allocator type
 * \threadsafe{safe,safe}
 * \test in file test_shared_hash.cpp */
template <impl::allocator A>
class basic_shared_hash: public impl::basic_shared_hash_base<A> {
public:
    //! It marks the object mt-safe.
    /*! \copydetails basic_value_object<A>::basic_value_object() */
    basic_shared_hash(typename basic_shared_hash::tag t,
        std::shared_ptr<const typename basic_shared_hash::method_table> methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node);
};

} // namespace threadscript
