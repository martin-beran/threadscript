#pragma once

/*! \file
 * \brief The main header file of the ThreadScript interpreter
 *
 * This header must be included by each program that embeds ThreadScript.
 */

//! \cond
#include "threadscript/config.hpp"
//! \endcond
#include "threadscript/virtual_machine.hpp"

//! The top-level namespace of ThreadScript
namespace threadscript {

//! The configured allocator for type \c void.
/*! It is passed to virtual_machine and other classes, which then rebind it to
 * other allocated types. */
using vm_allocator = config::allocator_type<void>;

//! The virtual machine class using the configured allocator
using virtual_machine = basic_virtual_machine<vm_allocator>;
extern template class basic_virtual_machine<vm_allocator>;

//! The thread state class using the configured allocator
using state = basic_state<vm_allocator>;
extern template class basic_state<vm_allocator>;

} // namespace threadscript
