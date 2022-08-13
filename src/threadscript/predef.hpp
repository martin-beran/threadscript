#pragma once

/*! \file
 * \brief Implementation of predefined built-in symbols. 
 */

#include "threadscript/symbol_table.hpp"
#include "threadscript/vm_data.hpp"

namespace threadscript {

//! Namespace for implementation of predefined built-in (C++ native) symbols
/*! This namespace contains predefined function implementation in classes
 * derived from threadscript::basic_value_native_fun. The class names have
 * prefix \c f_ in order to not collide with C++ keywords. It also contains
 * definitions of any predefined variables.
 *
 * Declarations in this namespace should be kept in the lexicographical order.
 * \test in file test_predef.cpp */
namespace predef {

//! Implementation of function \c print.
template <impl::allocator A>
class f_print {
};

} // namespace predef

//! Creates a new symbol table containing predefined built-in symbols.
/*! \tparam A an allocator type
 * \param[in] alloc the allocator used for creating the symbol table
 * \return a newly creating symbol table containing predefined built-in C++
 * native symbols defined in namespace threadscript::predef. */
template <impl::allocator A>
std::shared_ptr<basic_symbol_table<A>> predef_symbols(const A& alloc);

//! Adds default predefined built-in symbols to a symbol table.
/*! It adds built-in C++ native symbols defined in namespace
 * threadscript::predef into \a sym. It is usually used to initialize a symbol
 * table that will be set as the global shared symbol table
 * basic_virtual_machine::sh_vars. The function does nothing if \a sym is \c
 * nullptr.
 * \tparam A an allocator type
 * \param[in] sym a symbol table
 * \param[in] replace if \c false, any existing symbol with a name equal to
 * a name to be added is left unchanged; if \c true, any such symbol is
 * replaced by the default value.
 * \return \a sym */
template <impl::allocator A> std::shared_ptr<basic_symbol_table<A>>
add_predef_symbols(std::shared_ptr<basic_symbol_table<A>> sym,
                    bool replace = false);

} // namespace threadscript
