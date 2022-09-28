#pragma once

/*! \file
 * \brief A vector class that can be modified by multiple threads.
 */

#include "threadscript/vm_data.hpp"

namespace threadscript {

template <impl::allocator A> class basic_shared_vector;

namespace impl {
//! The name of shared_vector
inline constexpr char name_shared_vector[] = "shared_vector";
//! The base class of basic_shared_vector
/*! \tparam A an allocator type */
template <allocator A> using basic_shared_vector_base =
    basic_value_object<basic_shared_vector<A>, name_shared_vector, A>;
} // namespace impl

//! A thread-safe vector class
/*! Unlike basic_value_vector, all objects of this class are marked thread-safe
 * and can be accessed and modified by multiple threads simultaneously. The
 * storage for vector data is provided by a_basic_vector and its capacity is
 * managed automatically using the same algorithm as for basic_value_vector, as
 * described in basic_value_vector::value().
 *
 * Methods:
 * \snippet shared_vector_impl.hpp methods
 * \tparam A an allocator type
 * \threadsafe{safe,safe}
 * \test in file test_shared_vector.cpp */
template <impl::allocator A>
class basic_shared_vector: public impl::basic_shared_vector_base<A> {
public:
    //! It marks the object mt-safe.
    /*! \copydetails basic_value_object<A>::basic_value_object() */
    basic_shared_vector(typename basic_shared_vector::tag t,
        std::shared_ptr<const typename basic_shared_vector::method_table>
            methods,
        typename threadscript::basic_state<A>& thread,
        typename threadscript::basic_symbol_table<A>& l_vars,
        const typename threadscript::basic_code_node<A>& node);
    //! \copydoc basic_value_object::init_methods()
    [[nodiscard]] static
    typename basic_shared_vector::method_table init_methods();
private:
    //! Gets or sets a vector element.
    /*! Vector indices start at 0. If an index greater than the greatest
     * existing element is specified when setting a vector element, the vector
     * is extended to \c idx+1 arguments and elements between the previous last
     * element and \a idx are set to \c null. 
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     *     \arg \c idx -- an index of type \c int or \c unsigned
     *     \arg \c value -- (optional) if used, it is set as the element at \a
     *     idx; if missing, the element at \a idx is returned; it may be \c null
     * \return the existing (for get) or the new (for set) element at \a idx
     * \throw exception::op_narg if the number of arguments (incl. \a
     * method_name) is not 2 or 3
     * \throw exception::value_null if \a idx is \c null
     * \throw exception::value_type if \a idx is not of type \c int or \c
     * unsigned
     * \throw exception::value_out_of_range if \a idx is negative, or greater
     * or equal to \link a_basic_vector a_basic_vector::max_size()\endlink, or
     * (only when \a value is not used) greater than the greatest existing index
     * \throw exception::value_mt_unsafe if \a value is not mt-safe */
    typename basic_shared_vector::value_ptr
    at(typename threadscript::basic_state<A>& thread,
       typename threadscript::basic_symbol_table<A>& l_vars,
       const typename threadscript::basic_code_node<A>& node);
    //! Removes elements.
    /*! Elements from index \a idx to the end of the vector are deleted and the
     * vector is shrinked to the first \a idx elements. If \a idx is greater or
     * equal to the size of the vector then nothing is deleted and the vector
     * size remains unchanged. If called without argument \a idx, all elements
     * are removed.
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     *     \arg \c idx -- (optional) an index of type \c int or \c unsigned
     * \return \c null
     * \throw exception::op_narg if the number of arguments (incl. \a
     * method_name) is not 1 or 2
     * \throw exception::value_null if \a idx is \c null
     * \throw exception::value_type if \a idx does not have type \c int or \c
     * unsigned
     * \throw exception::value_out_of_range if \a idx is negative */
    typename basic_shared_vector::value_ptr
    erase(typename threadscript::basic_state<A>& thread,
          typename threadscript::basic_symbol_table<A>& l_vars,
          const typename threadscript::basic_code_node<A>& node);
    //! Gets the number of elements.
    /*! \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame
     * \param[in] node the code node, with method call arguments:
     *     \arg \c method_name
     * \return the number of elements in the vector (an \c unsigned value)
     * \throw exception::op_narg if the number of arguments (incl. \a
     * method_name) is not 1 */
    typename basic_shared_vector::value_ptr
    size(typename threadscript::basic_state<A>& thread,
         typename threadscript::basic_symbol_table<A>& l_vars,
         const typename threadscript::basic_code_node<A>& node);
    //! Storage of vector content
    a_basic_vector<typename basic_shared_vector::value_ptr, A> data;
    //! The mutex for synchronizing access from threads
    std::mutex mtx;
};

} // namespace threadscript
