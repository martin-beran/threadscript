#pragma once

/*! \file
 * \brief The main header file of the ThreadScript interpreter
 *
 * This header must be included by each program that embeds ThreadScript.
 */

#include "threadscript/configure.hpp"
#include "threadscript/channel.hpp"
#include "threadscript/code.hpp"
#include "threadscript/code_builder_impl.hpp"
#include "threadscript/code_parser.hpp"
#include "threadscript/predef.hpp"
#include "threadscript/shared_hash.hpp"
#include "threadscript/shared_vector.hpp"
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

/*** threadscript/channel.hpp ************************************************/

//! The channel using the configured allocator
using channel = basic_channel<allocator_any>;
extern template class basic_value_object<basic_channel<allocator_any>,
    threadscript::impl::name_channel, allocator_any>;
extern template class basic_channel<allocator_any>;

/*** threadscript/code.hpp ***************************************************/

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

/*** threadscript/code_builder_impl.hpp **************************************/

//! The basic_script_builder_impl using the configured allocator
using script_builder_impl = basic_script_builder_impl<allocator_any>;
extern template class basic_script_builder_impl<allocator_any>;

/*** threadscript/code_parser.hpp ********************************************/

//! The script parser using the configured allocator
/*! Documentation of the primary template function applies, except that \a A is
 * fixed to allocator_any:
 *
 * \copydetails parse_code(const A&, std::string_view, std::string_view,
 * std::string_view, parser::context::trace_t) */
extern template script::script_ptr
parse_code<allocator_any>(const allocator_any& alloc, std::string_view src,
                          std::string_view file, std::string_view syntax,
                          parser::context::trace_t trace);

//! The script parser using the configured allocator
/*! Documentation of the primary template function applies, except that \a A is
 * fixed to allocator_any:
 *
 * \copydetails parse_code_stream(const A&, std::istream&, std::string_view,
 * std::string_view, parser::context::trace_t) */
extern template script::script_ptr
parse_code_stream<allocator_any>(const allocator_any& alloc, std::istream& is,
                                 std::string_view file, std::string_view syntax,
                                 parser::context::trace_t trace);

//! The script parser using the configured allocator
/*! Documentation of the primary template function applies, except that \a A is
 * fixed to allocator_any:
 *
 * \copydetails parse_code_file(const A&, std::string_view, std::string_view,
 * parser::context::trace_t) */
extern template script::script_ptr
parse_code_file<allocator_any>(const allocator_any& alloc,
                               std::string_view file, std::string_view syntax,
                               parser::context::trace_t trace);

/*** threadscript/predef.hpp *************************************************/

//! Creates a new symbol table containing predefined built-in symbols.
/*! Documentation of the primary template function applies, except that \a A is
 * fixed to allocator_any:
 *
 * \copydetails predef_symbols(const A&) */
extern template std::shared_ptr<basic_symbol_table<allocator_any>>
predef_symbols(const allocator_any& alloc);

//! Adds default predefined built-in symbols to a symbol table.
/*! Documentation of the primary template function applies, except that \a A is
 * fixed to allocator_any:
 *
 * \copydetails add_predef_symbols(std::shared_ptr<basic_symbol_table<A>>,bool)
 */
extern template std::shared_ptr<basic_symbol_table<allocator_any>>
add_predef_symbols<allocator_any>(
                        std::shared_ptr<basic_symbol_table<allocator_any>> sym,
                        bool replace);

//! Registers constructor of predefined built-in classes to a symbol table.
/*! Documentation of the primary template function applies, except that \a A is
 * fixed to allocator_any:
 *
 * \copydetails add_predef_objects(std::shared_ptr<basic_symbol_table<A>>,bool)
 */
extern template std::shared_ptr<basic_symbol_table<allocator_any>>
add_predef_objects<allocator_any>(
                        std::shared_ptr<basic_symbol_table<allocator_any>> sym,
                        bool replace);

namespace predef {

extern template class f_add<allocator_any>;
extern template class f_and<allocator_any>;
extern template class f_and_r<allocator_any>;
extern template class f_at<allocator_any>;
extern template class f_bool<allocator_any>;
extern template class f_clone<allocator_any>;
extern template class f_contains<allocator_any>;
extern template class f_div<allocator_any>;
extern template class f_eq<allocator_any>;
extern template class f_erase<allocator_any>;
extern template class f_fun<allocator_any>;
extern template class f_ge<allocator_any>;
extern template class f_gt<allocator_any>;
extern template class f_gvar<allocator_any>;
extern template class f_hash<allocator_any>;
extern template class f_if<allocator_any>;
extern template class f_int<allocator_any>;
extern template class f_is_mt_safe<allocator_any>;
extern template class f_is_null<allocator_any>;
extern template class f_keys<allocator_any>;
extern template class f_le<allocator_any>;
extern template class f_lt<allocator_any>;
extern template class f_mod<allocator_any>;
extern template class f_mt_safe<allocator_any>;
extern template class f_mul<allocator_any>;
extern template class f_ne<allocator_any>;
extern template class f_not<allocator_any>;
extern template class f_or<allocator_any>;
extern template class f_or_r<allocator_any>;
extern template class f_print<allocator_any>;
extern template class f_is_same<allocator_any>;
extern template class f_seq<allocator_any>;
extern template class f_size<allocator_any>;
extern template class f_sub<allocator_any>;
extern template class f_substr<allocator_any>;
extern template class f_throw<allocator_any>;
extern template class f_try<allocator_any>;
extern template class f_type<allocator_any>;
extern template class f_unsigned<allocator_any>;
extern template class f_var<allocator_any>;
extern template class f_vector<allocator_any>;
extern template class f_while<allocator_any>;

} // namespace predef

/*** threadscript/shared_hash.hpp ********************************************/

//! The shared hash using the configured allocator
using shared_hash = basic_shared_hash<allocator_any>;
extern template class basic_value_object<basic_shared_hash<allocator_any>,
    threadscript::impl::name_shared_hash, allocator_any>;
extern template class basic_shared_hash<allocator_any>;

/*** threadscript/shared_vector.hpp ******************************************/

//! The shared vector using the configured allocator
using shared_vector = basic_shared_vector<allocator_any>;
extern template class basic_value_object<basic_shared_vector<allocator_any>,
    threadscript::impl::name_shared_vector, allocator_any>;
extern template class basic_shared_vector<allocator_any>;

/*** threadscript/symbol_table.hpp *******************************************/

//! The symbol table using the configured allocator
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

//! The \ref basic_value_vector using the configured allocator
using value_vector = basic_value_vector<allocator_any>;
extern template class basic_typed_value<value_vector,
    a_basic_vector<typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_vector, allocator_any>;
extern template class basic_value_vector<allocator_any>;

//! The \ref basic_value_hash using the configured allocator
using value_hash = basic_value_hash<allocator_any>;
extern template class basic_typed_value<value_hash,
    a_basic_hash<a_string, typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_hash, allocator_any>;
extern template class basic_value_hash<allocator_any>;

//! The \ref basic_value_object using the configured allocator
template <class Object, str_literal Name>
using value_object = basic_value_object<Object, Name, allocator_any>;

} // namespace threadscript
