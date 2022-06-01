/*! \file
 * \brief The implementation part of threadscript.hpp
 */

#include "threadscript/threadscript.hpp"
#include "threadscript/symbol_table_impl.hpp"
#include "threadscript/vm_data_impl.hpp"

namespace threadscript {

template class basic_virtual_machine<allocator_any>;
template class basic_state<allocator_any>;

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

template class basic_typed_value<value_hash,
    a_basic_hash<a_string, typename basic_value<allocator_any>::value_ptr,
        allocator_any>,
    threadscript::impl::name_value_hash, allocator_any>;

template class basic_symbol_table<allocator_any>;

template class basic_code_node<allocator_any>;

template class basic_script<allocator_any>;

} // namespace threadscript
