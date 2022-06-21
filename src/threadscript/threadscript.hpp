#pragma once

/*! \file
 * \brief The main header file of the ThreadScript interpreter
 *
 * This header must be included by each program that embeds ThreadScript.
 */

#include "threadscript/configure.hpp"
#include "threadscript/code.hpp"
#include "threadscript/symbol_table.hpp"
#include "threadscript/virtual_machine.hpp"
#include "threadscript/vm_data.hpp"

//! The top-level namespace of ThreadScript
namespace threadscript {

//! This namespace contains various implementation details
namespace impl {
} // namespace impl

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Keep declarations and explicit instantiations grouped by files containing
// primary templates and keep groups lexicographically ordered by file name.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*** threadscript/allocated.hpp **********************************************/

//! A string type using the configured allocator
using a_string = a_basic_string<allocator_any>;

//! A vector type using the configured allocator
/*! \tparam T a type of vector elements */
template <class T> using a_vector = a_basic_vector<T, allocator_any>;

//! A deque type using the configured allocator
/*! \tparam T a type of deque elements */
template <class T> using a_deque = a_basic_deque<T, allocator_any>;

/*** threadscript/code.http **************************************************/

//! The \ref basic_code_node using the configured allocator
using code_node = basic_code_node<allocator_any>;
extern template class basic_code_node<allocator_any>;
//! \cond
extern template
std::ostream& operator<< <allocator_any>(std::ostream&,
                                         const basic_code_node<allocator_any>&);
//! \endcond

//! The \ref basic_script using the configured allocator
using script = basic_script<allocator_any>;
extern template class basic_script<allocator_any>;
//! \cond
extern template
std::ostream& operator<< <allocator_any>(std::ostream&,
                                         const basic_script<allocator_any>&);
//! \endcond

//! The \ref basic_value_function using the configured allocator
using value_function = basic_value_function<allocator_any>;
extern template class basic_typed_value<basic_value_function<allocator_any>,
    std::shared_ptr<code_node>, threadscript::impl::name_value_function,
    allocator_any>;
extern template class basic_value_function<allocator_any>;

//! The \ref basic_value_script using the configured allocator
using value_script = basic_value_script<allocator_any>;
extern template class basic_typed_value<basic_value_script<allocator_any>,
    std::shared_ptr<script>, threadscript::impl::name_value_script,
    allocator_any>;
extern template class basic_value_script<allocator_any>;

//! The \ref basic_value_native_fun using the configured allocator
using value_native_fun = basic_value_native_fun<allocator_any>;
extern template class basic_typed_value<basic_value_native_fun<allocator_any>,
    threadscript::impl::empty, threadscript::impl::name_value_native_fun,
    allocator_any>;
extern template class basic_value_native_fun<allocator_any>;

/*** threadscript/symbol_table.hpp *******************************************/

//! The symbol_table using the configured allocator
using symbol_table = basic_symbol_table<allocator_any>;
extern template class basic_symbol_table<allocator_any>;

/*** threadscript/virtual_machine.hpp ****************************************/

//! The virtual machine class using the configured allocator
using virtual_machine = basic_virtual_machine<allocator_any>;
extern template class basic_virtual_machine<allocator_any>;

//! The thread state class using the configured allocator
using state = basic_state<allocator_any>;
extern template class basic_state<allocator_any>;

/*** threadscript/vm_data.hpp ************************************************/

//! The \ref basic_value class using the configured allocator
using value = basic_value<allocator_any>;
extern template class basic_value<allocator_any>;

//! The \ref basic_value_bool using the configured allocator
using value_bool = basic_value_bool<allocator_any>;
extern template class basic_typed_value<value_bool, bool,
    threadscript::impl::name_value_bool, allocator_any>;
extern template class basic_value_bool<allocator_any>;

//! The \ref basic_value_int using the configured allocator
using value_int = basic_value_int<allocator_any>;
extern template class basic_typed_value<value_int, config::value_int_type,
    threadscript::impl::name_value_int, allocator_any>;
extern template class basic_value_int<allocator_any>;

//! The \ref basic_value_unsigned using the configured allocator
using value_unsigned = basic_value_unsigned<allocator_any>;
extern template class basic_typed_value<value_unsigned,
    config::value_unsigned_type,
    threadscript::impl::name_value_unsigned, allocator_any>;
extern template class basic_value_unsigned<allocator_any>;

//! The \ref basic_value_string using the configured allocator
using value_string = basic_value_string<allocator_any>;
extern template class basic_typed_value<value_string,
    a_basic_string<allocator_any>, threadscript::impl::name_value_string,
    allocator_any>;
extern template class basic_value_string<allocator_any>;

//! The \ref basic_value_array using the configured allocator
using value_array = basic_value_array<allocator_any>;
extern template class basic_typed_value<value_array,
    a_basic_vector<typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_array, allocator_any>;
extern template class basic_value_array<allocator_any>;

//! The \ref basic_value_hash using the configured allocator
using value_hash = basic_value_hash<allocator_any>;
extern template class basic_typed_value<value_hash,
    a_basic_hash<a_string, typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_hash, allocator_any>;
extern template class basic_value_hash<allocator_any>;

} // namespace threadscript
