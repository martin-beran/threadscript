#pragma once

/*! \file
 * \brief The main header file of the ThreadScript interpreter
 *
 * This header must be included by each program that embeds ThreadScript.
 */

#include "threadscript/config.hpp"
#include "threadscript/config_default.hpp"
#include "threadscript/virtual_machine.hpp"
#include "threadscript/vm_data.hpp"

//! The top-level namespace of ThreadScript
namespace threadscript {

//! This namespace contains various implementation details
namespace impl {
} // namespace impl

//! The virtual machine class using the configured allocator
using virtual_machine = basic_virtual_machine<allocator_any>;
extern template class basic_virtual_machine<allocator_any>;

//! The thread state class using the configured allocator
using state = basic_state<allocator_any>;
extern template class basic_state<allocator_any>;

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
extern template class basic_typed_value<value_string, std::string,
    threadscript::impl::name_value_string, allocator_any>;
extern template class basic_value_string<allocator_any>;

} // namespace threadscript
