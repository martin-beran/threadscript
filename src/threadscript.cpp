/*! \file
 * \brief The implementation part of threadscript.hpp
 */

#include "threadscript/threadscript.hpp"
#include "threadscript/symbol_table_impl.hpp"
#include "threadscript/code_impl.hpp"
#include "threadscript/virtual_machine_impl.hpp"
#include "threadscript/vm_data_impl.hpp"

namespace threadscript {

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Keep declarations and explicit instantiations grouped by files containing
// primary templates and keep groups lexicographically ordered by file name.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*** threadscript/code.http **************************************************/

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

template class basic_typed_value<basic_value_native_fun<allocator_any>,
    threadscript::impl::empty, threadscript::impl::name_value_native_fun,
    allocator_any>;
template class basic_value_native_fun<allocator_any>;

/*** threadscript/code_builder_impl.hpp **************************************/

template class basic_script_builder_impl<allocator_any>;

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

template class basic_typed_value<value_array,
    a_basic_vector<typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_array, allocator_any>;
template class basic_value_array<allocator_any>;

template class basic_typed_value<value_hash,
    a_basic_hash<a_string, typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_hash, allocator_any>;
template class basic_value_hash<allocator_any>;

} // namespace threadscript
