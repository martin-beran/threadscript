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
 * and can be accessed and modified by multiple threads simultaneously. The
 * storage for hash data is provided by a_basic_hash and its capacity is
 * managed automatically using the same algorithm as for basic_value_hash, as
 * described in basic_value_hash::value().
 *
 * Methods:
 * \snippet shared_hash_impl.hpp methods
 * \tparam A an allocator type
 * \threadsafe{safe,safe}
 * \test in file test_shared_hash.cpp */
template <impl::allocator A>
class basic_shared_hash final: public impl::basic_shared_hash_base<A> {
public:
    //! It marks the object mt-safe.
    /*! \copydetails basic_value_object<A>::basic_value_object() */
    basic_shared_hash(typename basic_shared_hash::tag t,
        std::shared_ptr<const typename basic_shared_hash::method_table> methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node);
    //! \copydoc basic_value_object::init_methods()
    [[nodiscard]] static
    typename basic_shared_hash::method_table init_methods();
private:
    //! Gets or sets a hash element.
    /*! \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     *     \arg \c key -- a key of an element; it has type \c string
     *     \arg \c value -- (optional) if used, it is set as the element with
     *     \a key; if missing, the element with \a key is returned; it may be
     *     \c null
     * \return the existing (for get) or the new (for set) element with \a key
     * \throw exception::op_narg if the number of arguments (incl. \a
     * method_name) is not 2 or 3
     * \throw exception::value_null if \a key is \c null
     * \throw exception::value_type if \a key is not of type \c string
     * \throw exception::value_out_of_range if called without \a value and an
     * element with \a key does not existing
     * \throw exception::value_mt_unsafe if \a value is not mt-safe */
    typename basic_shared_hash::value_ptr
    at(typename threadscript::basic_state<A>& thread,
       typename threadscript::basic_symbol_table<A>& l_vars,
       const typename threadscript::basic_code_node<A>& node);
    //! Tests if the hash contains an element with \a key.
    /*! \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     *     \arg \c key -- a key of an element; it has type \c string
     * \return \c true if the hash contains an element with \a key; \c false
     * otherwise
     * \throw exception::op_narg if the number of arguments (incl. \a
     * method_name) is not 2
     * \throw exception::value_null if \a key is \c null
     * \throw exception::value_type if \a key is not of type \c string */
    typename basic_shared_hash::value_ptr
    contains(typename threadscript::basic_state<A>& thread,
             typename threadscript::basic_symbol_table<A>& l_vars,
             const typename threadscript::basic_code_node<A>& node);
    //! Removes elements.
    /*! If called without argument \a key, all elements are removed. Otherwise,
     * element with \a key is removed. It there is no element with \a key,
     * nothing is deleted and the hash remains unchanged.
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     *     \arg \c key -- (optional) a key of type \c string
     * \return \c null
     * \throw exception::op_narg if the number of arguments (incl. \c
     * method_name is not 1 or 2
     * \throw exception::value_null if \a key is \c null
     * \throw exception::value_type if \a key does not have type \c string */
    typename basic_shared_hash::value_ptr
    erase(typename threadscript::basic_state<A>& thread,
          typename threadscript::basic_symbol_table<A>& l_vars,
          const typename threadscript::basic_code_node<A>& node);
    //! Gets a \c vector (not \c shared_vector) for keys.
    /*! The elements of the returned vector, but not the vector itself, are
     * thread-safe (function predef::f_is_mt_safe returns \c true for them).
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     * \return a \c vector containing lexicographically sorted keys of the hash
     * \throw exception::op_narg if the number of arguments is not 1 */
    typename basic_shared_hash::value_ptr
    keys(typename threadscript::basic_state<A>& thread,
         typename threadscript::basic_symbol_table<A>& l_vars,
         const typename threadscript::basic_code_node<A>& node);
    //! Gets the number of elements.
    /*! \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     * \return the number of elements in the hash (an \c unsigned value)
     * \throw exception::op_narg if the number of arguments (incl. \a
     * method_name) is not 1 */
    typename basic_shared_hash::value_ptr
    size(typename threadscript::basic_state<A>& thread,
         typename threadscript::basic_symbol_table<A>& l_vars,
         const typename threadscript::basic_code_node<A>& node);
    //! Storage of hash content
    a_basic_hash<a_basic_string<A>, typename basic_shared_hash::value_ptr, A>
        data;
    //! The mutex for synchronizing access from threads
    std::mutex mtx;
};

} // namespace threadscript
