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
 * If a function, e.g., f_bool, has an (usually optional) parameter for storing
 * the result, a writable value must be passed in this argument. Otherwise,
 * exception::value_read_only is thrown.
 *
 * Classes in this namespace must be registered in variable \c factory in
 * function add_predef_symbols().
 *
 * Classes in this namespace should be \c final.
 *
 * Declarations in this namespace should be kept in the lexicographical order.
 * \test in file test_predef.cpp */
namespace predef {

//! Function \c bool
/*! Converts a value to \c bool. A \c bool value \c false yields \c false. Any
 * other non-null value yields \c true.
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val a value to be converted
 * \return \a val converted to \c bool
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a val is \c nullptr */
template <impl::allocator A>
class f_bool final: public basic_value_native_fun<f_bool<A>, A> {
    using basic_value_native_fun<f_bool<A>, A>::basic_value_native_fun;
public:
    //! Converts a value to basic_value_bool.
    /*! This is a helper function used by eval() and it should be used (for
     * consistency) by any other conversion to \c bool, for example, in f_if.
     * \param[in] thread the current thread
     * \param[in] val the value to be converted
     * \return \c false if \a val is basic_value_bool containing \c false;
     * \c true otherwise
     * \throw exception::value_null if \a val is \c nullptr */
    static bool convert(basic_state<A>& thread,
                        typename basic_value<A>::value_ptr val);
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c clone
/*! Creates a copy of a value. 
 * \param val a value to be copied
 * \return a writable copy of \a val
 * \throw exception::op_narg if the number of arguments is not 1
 * \throw exception::value_null if \a val is \c nullptr */
template <impl::allocator A>
class f_clone final: public basic_value_native_fun<f_clone<A>, A> {
    using basic_value_native_fun<f_clone<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c if
/*! A conditional statement.
 * \param cond converted to \c bool by f_bool::convert().
 * \param then evaluated if \a cond is \c true
 * \param else (optional) evaluated if \a cond is \c false
 * \return the result of the evaluated branch; \c null if \a cond is \c false
 * and the else-branch is missing
 * \throw exception::op_narg if the number of arguments is not 2 or 3 */
template <impl::allocator A>
class f_if final: public basic_value_native_fun<f_if<A>, A> {
    using basic_value_native_fun<f_if<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c is_mt_safe
/*! Tests if a value is marked as thread-safe and may be shared among threads.
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val a value to be tested
 * \return \c true if \a val is thread-safe; \c false otherwise
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a val is \c nullptr */
template <impl::allocator A>
class f_is_mt_safe final: public basic_value_native_fun<f_is_mt_safe<A>, A> {
    using basic_value_native_fun<f_is_mt_safe<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c is_null
/*! Tests if a value is \c null.
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val a value to be tested
 * \return \c true if \a val is \c null; \c false otherwise
 * \throw exception::op_narg if the number of arguments is not 1 or 2 */
template <impl::allocator A>
class f_is_null final: public basic_value_native_fun<f_is_null<A>, A> {
    using basic_value_native_fun<f_is_null<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c is_same
/*! Tests if two values are the same, that is, if they reference the same
 * location in memory.
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val1 the first value to be tested
 * \param val2 the second value to be tested
 * \return \c true if \a val1 and \a val2 refer to the same value; \c false
 * otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c nullptr */
template <impl::allocator A>
class f_is_same final: public basic_value_native_fun<f_is_same<A>, A> {
    using basic_value_native_fun<f_is_same<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c mt_safe
/*! Sets a value as thread-safe, so that it becomes read-only and can be shared
 * among threads.
 * \param val a value to modified
 * \return \a val
 * \throw exception::op_narg if the number of arguments is not 1
 * \throw exception::value_null if \a val is \c nullptr
 * \throw exception::value_mt_unsafe if this value does not satisfy conditions
 * for being thread-safe */
template <impl::allocator A>
class f_mt_safe final: public basic_value_native_fun<f_mt_safe<A>, A> {
    using basic_value_native_fun<f_mt_safe<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c print
/*! It writes all its arguments atomically to the standard output, which can be
 * redirected to any \c std::ostream by basic_state::std_out or
 * basic_virtual_machine::std_out. A value is written by its member function
 * basic_value::write(). If a value is \c null, then the string \c "null" is
 * written. Output generated by a single call of \c print is not interleaved by
 * other output to the same stream.
 * \param val (0 or more) values to be written
 * \return \c null. */
template <impl::allocator A>
class f_print final: public basic_value_native_fun<f_print<A>, A> {
    using basic_value_native_fun<f_print<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c seq
/*! It evaluates all its arguments sequentially. This is essentially equivalent
 * to a block of commands in other programming languages.
 * \param cmd (0 or more) commands or functions to be executed
 * \return \c null if called without arguments; the result of the last argument
 * otherwise */
template <impl::allocator A>
class f_seq final: public basic_value_native_fun<f_seq<A>, A> {
    using basic_value_native_fun<f_seq<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c type
/*! Gets a type name of a value.
 * \param result (optional) if exists and has type \c string, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val get the type of this value
 * \return the type name of \a val
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a val is \c nullptr */
template <impl::allocator A>
class f_type final: public basic_value_native_fun<f_type<A>, A> {
    using basic_value_native_fun<f_type<A>, A>::basic_value_native_fun;
public:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c var
/*! It gets or sets a \a value of variable \a name. It assings a reference to
 * the \a value instead of copying it. Hence, the same value may be accessible
 * by several names.
 * \param name the variable name
 * \param value (optional) sets the variable to this value; without it, the
 * variable is unchanged
 * \return the value of the variable; the new value if \a value is used
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a name is \c null
 * \throw exception::value_type if \a name is not of type \c string
 * \throw exception::unknown_symbol if called with one argument and variable \a
 * name does not exist */
template <impl::allocator A>
class f_var final: public basic_value_native_fun<f_var<A>, A> {
    using basic_value_native_fun<f_var<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c while
/*! A while-loop. First, \a cond is tested. If \c true, then \a body is
 * evaluated and the whole cycle is repeated. If \a cond becomes \c false, the
 * \a body is no more executed and the loop terminates.
 * \param cond converted to \c bool by f_bool::convert()
 * \param body evaluated while \a cond is \c true
 * \return the result of the last evaluation of \a cond, or \c null if \a cond
 * is not evaluated at least once
 * \throw exception::op_narg if the number of arguments is not 2 */
template <impl::allocator A>
class f_while final: public basic_value_native_fun<f_while<A>, A> {
    using basic_value_native_fun<f_while<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
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
add_predef_symbols(std::shared_ptr<basic_symbol_table<A>> sym, bool replace);

} // namespace threadscript
