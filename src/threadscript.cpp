/*! \file
 * \brief The implementation part of threadscript.hpp
 */

#include "threadscript/threadscript.hpp"
#include "threadscript/symbol_table_impl.hpp"
#include "threadscript/code_impl.hpp"
#include "threadscript/code_parser_impl.hpp"
#include "threadscript/predef_impl.hpp"
#include "threadscript/virtual_machine_impl.hpp"
#include "threadscript/vm_data_impl.hpp"

namespace threadscript {

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Keep declarations and explicit instantiations grouped by files containing
// primary templates and keep groups lexicographically ordered by file name.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*** threadscript/code.hpp ***************************************************/

template class basic_code_node<allocator_any>;
//! \cond
template
std::ostream& operator<< <allocator_any>(std::ostream&,
                                         const basic_code_node<allocator_any>&);
//! \endcond

template class basic_script<allocator_any>;
//! \cond
template
std::ostream& operator<< <allocator_any>(std::ostream&,
                                         const basic_script<allocator_any>&);
//! \endcond

template class basic_typed_value<basic_value_function<allocator_any>,
    typename basic_script<allocator_any>::node_ptr,
    threadscript::impl::name_value_function,
    allocator_any>;
template class basic_value_function<allocator_any>;

template class basic_typed_value<basic_value_script<allocator_any>,
    std::shared_ptr<script>, threadscript::impl::name_value_script,
    allocator_any>;
template class basic_value_script<allocator_any>;

/*** threadscript/code_builder_impl.hpp **************************************/

template class basic_script_builder_impl<allocator_any>;

/*** threadscript/code_parser_impl.hpp ***************************************/

template script::script_ptr
parse_code<allocator_any>(const allocator_any& alloc, std::string_view src,
                          std::string_view file, std::string_view syntax,
                          parser::context::trace_t trace);

template script::script_ptr
parse_code_file<allocator_any>(const allocator_any& alloc,
                               std::string_view file, std::string_view syntax,
                               parser::context::trace_t trace);

/*** threadscript/predef.hpp *************************************************/

template std::shared_ptr<basic_symbol_table<allocator_any>>
predef_symbols(const allocator_any& alloc);

template std::shared_ptr<basic_symbol_table<allocator_any>>
add_predef_symbols<allocator_any>(
                        std::shared_ptr<basic_symbol_table<allocator_any>> sym,
                        bool replace);

namespace predef {

template class f_add<allocator_any>;
template class f_and<allocator_any>;
template class f_and_r<allocator_any>;
template class f_at<allocator_any>;
template class f_bool<allocator_any>;
template class f_clone<allocator_any>;
template class f_contains<allocator_any>;
template class f_div<allocator_any>;
template class f_eq<allocator_any>;
template class f_erase<allocator_any>;
template class f_ge<allocator_any>;
template class f_gt<allocator_any>;
template class f_hash<allocator_any>;
template class f_if<allocator_any>;
template class f_int<allocator_any>;
template class f_is_mt_safe<allocator_any>;
template class f_is_null<allocator_any>;
template class f_keys<allocator_any>;
template class f_le<allocator_any>;
template class f_lt<allocator_any>;
template class f_mod<allocator_any>;
template class f_mt_safe<allocator_any>;
template class f_mul<allocator_any>;
template class f_ne<allocator_any>;
template class f_not<allocator_any>;
template class f_or<allocator_any>;
template class f_or_r<allocator_any>;
template class f_print<allocator_any>;
template class f_is_same<allocator_any>;
template class f_seq<allocator_any>;
template class f_size<allocator_any>;
template class f_sub<allocator_any>;
template class f_type<allocator_any>;
template class f_unsigned<allocator_any>;
template class f_var<allocator_any>;
template class f_vector<allocator_any>;
template class f_while<allocator_any>;

} // namespace predef

/*** threadscript/symbol_table.hpp *******************************************/

template class basic_symbol_table<allocator_any>;

/*** threadscript/virtual_machine.hpp ****************************************/

template class basic_virtual_machine<allocator_any>;
template class basic_state<allocator_any>;

/*** threadscript/vm_data.hpp ************************************************/

template class basic_value<allocator_any>;

template class basic_typed_value<value_bool, bool,
    threadscript::impl::name_value_bool, allocator_any>;
template class basic_value_bool<allocator_any>;

template class basic_typed_value<value_int, config::value_int_type,
    threadscript::impl::name_value_int, allocator_any>;
template class basic_value_int<allocator_any>;

template class basic_typed_value<value_unsigned, config::value_unsigned_type,
    threadscript::impl::name_value_unsigned, allocator_any>;
template class basic_value_unsigned<allocator_any>;

template class basic_typed_value<value_string, a_basic_string<allocator_any>,
    threadscript::impl::name_value_string, allocator_any>;
template class basic_value_string<allocator_any>;

template class basic_typed_value<value_vector,
    a_basic_vector<typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_vector, allocator_any>;
template class basic_value_vector<allocator_any>;

template class basic_typed_value<value_hash,
    a_basic_hash<a_string, typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_hash, allocator_any>;
template class basic_value_hash<allocator_any>;

} // namespace threadscript
