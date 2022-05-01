/*! \file
 * \brief The implementation part of threadscript.hpp
 */

#include "threadscript/threadscript.hpp"
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

template class basic_typed_value<value_string, std::string,
    threadscript::impl::name_value_string, allocator_any>;
template class basic_value_string<allocator_any>;

} // namespace threadscript
