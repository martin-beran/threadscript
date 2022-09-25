#pragma once

/*! \file
 * \brief Implementation of predefined built-in symbols. 
 */

#include "threadscript/symbol_table.hpp"
#include "threadscript/vm_data.hpp"

#include <variant>

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
 * function add_predef_symbols(). A link to documentation of each class should
 * be added to the appropriate group in \ref Builtin_commands.
 *
 * Classes in this namespace should be \c final.
 *
 * Declarations in this namespace should be kept in the lexicographical order.
 * \test in file test_predef.cpp */
namespace predef {

//! Function \c add
/*! Numeric addition and string concatenation. Unsigned addition is done using
 * modulo arithmetic, signed overflow causes exception::op_overflow.
 * \param result (optional) if it exists and has the same type as \a val1 and
 * \a val2, the result is stored into it; otherwise, a new value is allocated
 * for the result
 * \param val1 the first operand, it must be \c int, \c unsigned, or \c string
 * \param val2 the second operand, it must have the same type as \a val1
 * \return \a val1 + \a val2 if they are \c int or \c unsigned; concatenation
 * of \a val1 and \a val2 if they are \c string
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 and \a val2 do not have the same
 * type or if their type is not \c int, \c unsigned, or \c string
 * \throw exception::op_overflow if \a val1 and \a val2 have type \c int and
 * overflow occurs
 * \note An alternative would be to allow more than two operands. String
 * concatenation could then first compute the final string length and use a
 * single allocation for the concatenated string. But it would need another
 * allocation for a vector of strings, because each argument may be evaluated
 * at most once. It would also need two variants of addition, one with the
 * result argument and one without it (like f_and and f_and_r). */
template <impl::allocator A>
class f_add final: public basic_value_native_fun<f_add<A>, A> {
    using basic_value_native_fun<f_add<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Common functionality of classes f_and and f_and_r
/*! \tparam A an allocator type */
template <impl::allocator A>
class f_and_base: public basic_value_native_fun<f_and_base<A>, A> {
    using basic_value_native_fun<f_and_base<A>, A>::basic_value_native_fun;
protected:
    //! Computes a result.
    /*! This is the common part of evaluation used by both f_and::eval() and
     * f_and_r::eval(). Arguments are the same as in eval():
     * \param[in] thread
     * \param[in] l_vars
     * \param[in] node
     * \param[in] begin index of the first input argument
     * \return the result of evaluation
     * \throw exception::value_null if any evaluated \a val is \c null */
    bool eval_impl(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                   const basic_code_node<A>& node, size_t begin = 0);
};

//! Function \c and
/*! Logical conjunction. It uses short-circuit evaluation, that is, arguments
 * are evaluated in order only until the final result is known (until the first
 * \c false or until all values are tested).
 * \param val (0 or more) input values converted to \c bool by
 * f_bool::convert()
 * \return \c true if all \a val are \c true (including an empty list of
 * arguments); \c false otherwise
 * \throw exception::value_null if any evaluated \a val is \c null */
template <impl::allocator A>
class f_and final: public f_and_base<A> {
    using f_and_base<A>::f_and_base;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c and_r
/*! It is similar to f_and, but the first argument is used for storing the
 * result.
 * \param result if it has type \c bool, the result is stored into it;
 * otherwise, a new value is allocated for the result
 * \param val (0 or more) input values converted to \c bool by
 * f_bool::convert()
 * \return \c true if all \c val are \c true (including an empty list of
 * arguments); \c false otherwise
 * \throw exception::op_narg if the number of arguments is 0
 * \throw exception::value_null if any evaluated \a val is \c null */
template <impl::allocator A>
class f_and_r final: public f_and_base<A> {
    using f_and_base<A>::f_and_base;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c at
/*! It gets or sets an element of a \c vector or a \c hash. Vector indices
 * start at 0. If an index greater than the greatest existing element is
 * specified when setting a vector element (calling with 3 arguments), the
 * vector is extended to <tt>idx+1</tt> arguments and elements between the
 * previous last element and \a idx are set to \c null.
 * \param container a value of type \c vector or \c hash
 * \param idx an index (of type \c int or \c unsigned for a \c vector), or a
 * key (of type \c string for a \c hash)
 * \param value (optional) if used, it is set as the element at \a idx; if
 * missing, the element at \a idx is returned; it may be be \c null
 * \return the existing (for get) or the new (for set) element at \a idx
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if the first or the second argument is \c null
 * \throw exception::value_type if \a container is not of type \c vector
 * or \c hash, or if \a idx is not of type \c int or \c unsigned (for \a
 * container of type \c vector) or \c string (for \a container of type \c hash)
 * \throw exception::value_out_of_range if a \c vector \a idx is negative or
 * greater or equal to \link a_basic_vector a_basic_vector::max_size()\endlink
 * or (only when \a value is not used) greater than the greatest existing
 * index; or if called without \a value for a \c hash and key \a idx does not
 * exist
 * \throw exception::value_read_only if trying to set an element in an
 * read-only \c vector or \c hash */
template <impl::allocator A>
class f_at final: public basic_value_native_fun<f_at<A>, A> {
    using basic_value_native_fun<f_at<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c bool
/*! Converts a value to \c bool. A \c bool value \c false yields \c false. Any
 * other non-null value yields \c true.
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val a value to be converted
 * \return \a val converted to \c bool
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a val is \c null */
template <impl::allocator A>
class f_bool final: public basic_value_native_fun<f_bool<A>, A> {
    using basic_value_native_fun<f_bool<A>, A>::basic_value_native_fun;
public:
    //! Converts a value to basic_value_bool.
    /*! This is a helper function used by eval() and it should be used (for
     * consistency) by any other conversion to \c bool, for example, in f_if.
     * \param[in] val the value to be converted
     * \return \c false if \a val is basic_value_bool containing \c false;
     * \c true otherwise
     * \throw exception::value_null if \a val is \c null */
    static bool convert(typename basic_value<A>::value_ptr val);
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
 * \throw exception::value_null if \a val is \c null */
template <impl::allocator A>
class f_clone final: public basic_value_native_fun<f_clone<A>, A> {
    using basic_value_native_fun<f_clone<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c contains
/*! Tests if \a hash contains an element with \a key.
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param hash the hash searched for \a key
 * \param key the searched key in \a hash
 * \return \c true if \a hash contains an element with \a key; \c false
 * otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a hash or \a key is \c null
 * \throw exception::value_type if \a hash does not have type \c hash or
 * \a key does not have type \c string */
template <impl::allocator A>
class f_contains final: public basic_value_native_fun<f_contains<A>, A> {
    using basic_value_native_fun<f_contains<A>, A>::basic_value_native_fun;
public:
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Common functionality of classes f_div and f_mod
/*! \tparam A an allocator type */
template <impl::allocator A>
class f_div_base: public basic_value_native_fun<f_div_base<A>, A> {
    using basic_value_native_fun<f_div_base<A>, A>::basic_value_native_fun;
protected:
    //! Computes a result.
    /*! This is the common part of evaluation used by both f_div::eval() and
     * f_mod::eval(). Arguments are the same as in eval():
     * \param[in] thread
     * \param[in] l_vars
     * \param[in] node
     * \param[in] div division if \c true, remainder if \c false
     * \return the result of evaluation
     * \throw exception::runtime_error a class derived from this as described
     * in f_div documentation */
    typename basic_value<A>::value_ptr eval_impl(basic_state<A>& thread,
                                                 basic_symbol_table<A>& l_vars,
                                                 const basic_code_node<A>& node,
                                                 bool div);
};

//! Function \c div
/*! Numeric division. Unsigned division is done using modulo arithmetic, signed
 * overflow (only when dividing the minimum value by -1) causes
 * exception::op_overflow.
 * \param result (optional) if it exists and has the same type as \a val1 and
 * \a val2, the result is stored into it; otherwise, a new value is allocated
 * for the result
 * \param val1 the first operand, it must be \c int or \c unsigned
 * \param val2 the second operand, it must have the same type as \a val1
 * \return \a val1 / \a val2
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 and \a val2 do not have the same
 * type or if their type is not \c int or \c unsigned
 * \throw exception::op_div_zero if \a val2 is zero
 * \throw exception::op_overflow if \a val1 and \a val2 have type \c int and
 * overflow occurs, that is, if \a val1 is the minimum (negative) value and \a
 * val2 is -1 */
template <impl::allocator A>
class f_div final: public f_div_base<A> {
    using f_div_base<A>::f_div_base;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c eq
/*! Compares two values for equality. Unlike f_is_same, this function compares
 * the contents of values, not their location in memory. If both values are of
 * the same type, the rules of equality for that type apply. If the values are
 * of different types, the following rules apply:
 * \arg If one value is of type \c bool, the other is converted using
 * f_bool::convert() and the resulting \c bool value is used in comparison.
 * \arg Otherwise, if each value is \c int or \c unsigned, their numeric values
 * are compared.
 *
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val1 the first value to be compared
 * \param val2 the second value to be compared
 * \return \c true if \a val1 and \a val2 are equal; \c false otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 or \a val2 has a type other than \c
 * bool, \c int, \c unsigned, or \c string; also if \a val1 or \a val2 have
 * a different type combination than allowed by the rules above */
template <impl::allocator A>
class f_eq final: public basic_value_native_fun<f_eq<A>, A> {
    using basic_value_native_fun<f_eq<A>, A>::basic_value_native_fun;
public:
    //! Performs the equality comparison.
    /*! This is a helper function used by eval() and it should be used (for
     * consistency) by any other equality-comparing function, e.g., f_ne.
     * \param[in] val1 the first value to be compared
     * \param[in] val2 the second value to be compared
     * \return \c true if \a val1 and \a val2 are equal; \c false otherwise
     * \throw exception::value_null if \a val1 or \a val2 is \c null
     * \throw exception::value_type under the same conditions as specified for
     * class f_eq itself */
    static bool compare(typename basic_value<A>::value_ptr val1,
                        typename basic_value<A>::value_ptr val2);
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c erase
/*! Removes elements from a value of type \c vector or \c hash. If called
 * without argument \a idx, all elements are removed.
 *
 * If \a container has type \c vector then elements from index \a idx to the
 * end of the vector are deleted and the vector is shrinked to the first \a idx
 * elements. If \a idx is greater or equal to the size of the vector then
 * nothing is deleted and the vector size remains unchanged.
 *
 * If \a container has type \c hash then the element with key \a idx is
 * deleted. If \c hash does not contain an element with key \a idx then nothing
 * is deleted and the hash remains unchanged.
 * \param container a value of type \c vector or \c hash
 * \param idx (optional) an index (of type \c int or \c unsigned for a \c
 * vector), or a key (of type \c string for a \c hash)
 * \return \c null
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a container or \a idx is \c null
 * \throw exception::value_type if \a container does not have type \c vector
 * or \c hash, or if \a idx does not have type \c int or \c unsigned for a \c
 * vector, or if \a idx does not have type \c string for a \c hash
 * \throw exception::value_out_of_range if \a idx is negative
 * \throw exception::value_read_only if trying to remove an element from a
 * read-only \a container */
template <impl::allocator A>
class f_erase final: public basic_value_native_fun<f_erase<A>, A> {
    using basic_value_native_fun<f_erase<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c fun
/*! Defines a function. The function is stored in the global symbol table of
 * the current thread (basic_state::t_vars).
 * \param name the name of the function
 * \param body the body of the function, evaluated every time the function is
 * called
 * \return \c null (this is the return value of function definition command,
 * not of a function call)
 * \throw exception::op_narg if the number of arguments is not 2
 * \throw exception::value_null if \a name is \c null
 * \throw exception::value_type if \a name does not have type \c string */
template <impl::allocator A>
class f_fun final: public basic_value_native_fun<f_fun<A>, A> {
    using basic_value_native_fun<f_fun<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c ge
/*! Compares two values for greater-or-equal relation. Unlike f_is_same, this
 * function compares the contents of values, not their location in memory. If
 * both values are of the same type, the rules of ordering for that type apply.
 * If the values are of different types, the following rules apply:
 * \arg If one value is of type \c bool, the other is converted using
 * f_bool::convert() and the resulting \c bool value is used in comparison,
 * with \c false being less than \c true.
 * \arg Otherwise, if each value is \c int or \c unsigned, their numeric values
 * are compared.
 *
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val1 the first value to be compared
 * \param val2 the second value to be compared
 * \return \c true if \a val1 is greater or equal to \a val2; \c false otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 or \a val2 has a type other than \c
 * bool, \c int, \c unsigned, or \c string; also if \a val1 or \a val2 have
 * a different type combination than allowed by the rules above */
template <impl::allocator A>
class f_ge final: public basic_value_native_fun<f_ge<A>, A> {
    using basic_value_native_fun<f_ge<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c gt
/*! Compares two values for greater-than relation. Unlike f_is_same, this
 * function compares the contents of values, not their location in memory. If
 * both values are of the same type, the rules of ordering for that type apply.
 * If the values are of different types, the following rules apply:
 * \arg If one value is of type \c bool, the other is converted using
 * f_bool::convert() and the resulting \c bool value is used in comparison,
 * with \c false being less than \c true.
 * \arg Otherwise, if each value is \c int or \c unsigned, their numeric values
 * are compared.
 *
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val1 the first value to be compared
 * \param val2 the second value to be compared
 * \return \c true if \a val1 is greater thant \a val2; \c false otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 or \a val2 has a type other than \c
 * bool, \c int, \c unsigned, or \c string; also if \a val1 or \a val2 have
 * a different type combination than allowed by the rules above */
template <impl::allocator A>
class f_gt final: public basic_value_native_fun<f_gt<A>, A> {
    using basic_value_native_fun<f_gt<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c gvar
/*! It sets a \a value of global variable \a name of the current thread. That
 * is, it sets a variable in basic_state::t_vars. For getting a value of a
 * variable or setting a local variable, use f_var.
 * \param name the variable name
 * \param value sets the variable to this value
 * \return \c null
 * \throw exception::op_narg if the number of arguments is not 2
 * \throw exception::value_null if \a name if \c null
 * \throw exception::value_type if \a name is not of type \c string */
template <impl::allocator A>
class f_gvar final: public basic_value_native_fun<f_gvar<A>, A> {
    using basic_value_native_fun<f_gvar<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c hash
/*! It creates a new empty value of type \c hash.
 * \return the newly created empty \c hash
 * \throw exception::op_narg if the number of arguments is not 0 */
template <impl::allocator A>
class f_hash final: public basic_value_native_fun<f_hash<A>, A> {
    using basic_value_native_fun<f_hash<A>, A>::basic_value_native_fun;
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

//! Function \c int
/*! Converts a value to \c int. An \c int value is returned unchanged. An \c
 * uint value is converted using integral conversion rules of C++, that is,
 * using modulo arithmetic. A \c string value is interpreted as a decimal
 * number with optional leading sign.
 * \param result (optional) if exists and has type \c int, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val convert this value
 * \return \a val converted to type \c int
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a val is \c null
 * \throw exception::value_type if \a val is not of type \c int, \c unsigned,
 * or \c string
 * \throw exception::value_bad if string \a val does not contain a decimal
 * number
 * \throw exception::value_out_of_range if a string \a val contains a
 * correctly formated value out of range of type \c int */
template <impl::allocator A>
class f_int final: public basic_value_native_fun<f_int<A>, A> {
    using basic_value_native_fun<f_int<A>, A>::basic_value_native_fun;
public:
    //! Converts a string value to an integer.
    /*! \param[in] str a string to be converted
     * \param[in] sign whether a signed result is expected
     * \return \a str converted to the appropriate type according to \a sign
     * \throw exception::value_bad if \a str does not contain a decimal
     * number
     * \throw exception::value_out_of_range if \a str contains a correctly
     * formated value out of range of the target type */
    static std::variant<config::value_int_type, config::value_unsigned_type>
    from_string(const a_basic_string<A>& str, bool sign);
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
 * \throw exception::value_null if \a val is \c null */
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
 * \throw exception::value_null if \a val1 or \a val2 is \c null */
template <impl::allocator A>
class f_is_same final: public basic_value_native_fun<f_is_same<A>, A> {
    using basic_value_native_fun<f_is_same<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c keys
/*! Gets a vector of keys from a hash. The elements of the returned vectors,
 * but not the vector itself, are thread-safe (read-only, function f_is_mt_safe
 * returns \c true for them)
 * \param hash a hash
 * \return a vector containing lexicographically sorted keys from \a hash
 * \throw exception::op_narg if the number of arguments is not 1
 * \throw exception::value_null if \a hash is \c null
 * \throw exception::value_type if \a hash is not of type \c hash */
template <impl::allocator A>
class f_keys final: public basic_value_native_fun<f_keys<A>, A> {
    using basic_value_native_fun<f_keys<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c le
/*! Compares two values for less-or-equal relation. Unlike f_is_same, this
 * function compares the contents of values, not their location in memory. If
 * both values are of the same type, the rules of ordering for that type apply.
 * If the values are of different types, the following rules apply:
 * \arg If one value is of type \c bool, the other is converted using
 * f_bool::convert() and the resulting \c bool value is used in comparison,
 * with \c false being less than \c true.
 * \arg Otherwise, if each value is \c int or \c unsigned, their numeric values
 * are compared.
 *
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val1 the first value to be compared
 * \param val2 the second value to be compared
 * \return \c true if \a val1 is less or equal to \a val2; \c false otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 or \a val2 has a type other than \c
 * bool, \c int, \c unsigned, or \c string; also if \a val1 or \a val2 have
 * a different type combination than allowed by the rules above */
template <impl::allocator A>
class f_le final: public basic_value_native_fun<f_le<A>, A> {
    using basic_value_native_fun<f_le<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c lt
/*! Compares two values for less-than relation. Unlike f_is_same, this function
 * compares the contents of values, not their location in memory. If both
 * values are of the same type, the rules of ordering for that type apply. If
 * the values are of different types, the following rules apply:
 * \arg If one value is of type \c bool, the other is converted using
 * f_bool::convert() and the resulting \c bool value is used in comparison,
 * with \c false being less than \c true.
 * \arg Otherwise, if each value is \c int or \c unsigned, their numeric values
 * are compared.
 *
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val1 the first value to be compared
 * \param val2 the second value to be compared
 * \return \c true if \a val1 is less than \a val2; \c false otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 or \a val2 has a type other than \c
 * bool, \c int, \c unsigned, or \c string; also if \a val1 or \a val2 have
 * a different type combination than allowed by the rules above */
template <impl::allocator A>
class f_lt final: public basic_value_native_fun<f_lt<A>, A> {
    using basic_value_native_fun<f_lt<A>, A>::basic_value_native_fun;
public:
    //! Performs the less-than comparison.
    /*! This is a helper function used by eval() and it should be used (for
     * consistency) by any other ordering function, e.g., f_lt, f_le, f_gt,
     * f_ge.
     * \param[in] val1 the first value to be compared
     * \param[in] val2 the second value to be compared
     * \return \c true if \a val1 is less than \a val2; \c false otherwise
     * \throw exception::value_null if \a val1 or \a val2 is \c null
     * \throw exception::value_type under the same conditions as specified for
     * class f_eq itself */
    static bool compare(typename basic_value<A>::value_ptr val1,
                        typename basic_value<A>::value_ptr val2);
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c mod
/*! Numeric remainder (modulo) of division. It follows the C++ rules, therefore
 * it throws exceptions under the same conditions as f_div: Unsigned division
 * is done using modulo arithmetic, signed overflow (only when dividing the
 * minimum value by -1) causes exception::op_overflow. Division and remainder
 * satisfy the identity
 * \code
 * (a / b) * b + a % b == a
 * \endcode
 * \param result (optional) if it exists and has the same type as \a val1 and
 * \a val2, the result is stored into it; otherwise, a new value is allocated
 * for the result
 * \param val1 the first operand, it must be \c int or \c unsigned
 * \param val2 the second operand, it must have the same type as \a val1
 * \return \a val1 % \a val2
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 and \a val2 do not have the same
 * type or if their type is not \c int or \c unsigned
 * \throw exception::op_div_zero if \a val2 is zero
 * \throw exception::op_overflow if \a val1 and \a val2 have type \c int and
 * overflow occurs in division, that is, if \a val1 is the minimum (negative)
 * value and \a val2 is -1 */
template <impl::allocator A>
class f_mod final: public f_div_base<A> {
    using f_div_base<A>::f_div_base;
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
 * \throw exception::value_null if \a val is \c null
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

//! Function \c mul
/*! Numeric multiplication and string repetition. Unsigned multiplication is
 * done using modulo arithmetic, signed overflow causes exception::op_overflow.
 * If both arguments \a val1 and \a val2 are \c int or \c unsigned, then they
 * are multiplied. If one them is \c string, the other must be \c unsigned or a
 * nonnegative \c int and the string argument is repeated as many times as is
 * the value of the numeric argument.
 * \param result (optional) if it exists and has the same type as \a val1 and
 * \a val2, the result is stored into it; otherwise, a new value is allocated
 * for the result
 * \param val1 the first operand
 * \param val2 the second operand
 * \return \a val1 * \a val2 if they are \c int or \c unsigned; repeated of
 * the string argument otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 and \a val2 do not have an allowed
 * combination of types: either both \c int, both \c unsigned, or one \c string
 * and the other \c int or \c unsigned
 * \throw exception::op_overflow if \a val1 and \a val2 have type \c int and
 * overflow occurs; or if one argument is \c string and the other is a negative
 * \c int */
template <impl::allocator A>
class f_mul final: public basic_value_native_fun<f_mul<A>, A> {
    using basic_value_native_fun<f_mul<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c ne
/*! Compares two values for inequality. It is the negation of f_eq. Unlike
 * f_is_same, this function compares the contents of values, not their location
 * in memory. If both values are of the same type, the rules of equality for
 * that type apply. If the values are of different types, the following rules
 * apply:
 * \arg If one value is of type \c bool, the other is converted using
 * f_bool::convert() and the resulting \c bool value is used in comparison.
 * \arg Otherwise, if each value is \c int or \c unsigned, their numeric values
 * are compared.
 *
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val1 the first value to be compared
 * \param val2 the second value to be compared
 * \return \c true if \a val1 and \a val2 are not equal; \c false otherwise
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 or \a val2 has a type other than \c
 * bool, \c int, \c unsigned, or \c string; also if \a val1 or \a val2 have
 * a different type combination than allowed by the rules above */
template <impl::allocator A>
class f_ne final: public basic_value_native_fun<f_ne<A>, A> {
    using basic_value_native_fun<f_ne<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c not
/*! Negates a Boolean value.
 * \param result (optional) if exists and has type \c bool, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val a value converted to \c bool by f_bool::convert()
 * \return negated \a val (\c true if \a val is \c false; \c false if \a val is
 * \c true)
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a val is \c null */
template <impl::allocator A>
class f_not final: public basic_value_native_fun<f_not<A>, A> {
    using basic_value_native_fun<f_not<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Common functionality of classes f_or and f_or_r
/*! \tparam A an allocator type */
template <impl::allocator A>
class f_or_base: public basic_value_native_fun<f_or_base<A>, A> {
    using basic_value_native_fun<f_or_base<A>, A>::basic_value_native_fun;
protected:
    //! Computes a result.
    /*! This is the common part of evaluation used by both f_or::eval() and
     * f_or_r::eval(). Arguments are the same as in eval():
     * \param[in] thread
     * \param[in] l_vars
     * \param[in] node
     * \param[in] begin index of the first input argument
     * \return the result of evaluation
     * \throw exception::value_null if any evaluated \a val is \c null */
    bool eval_impl(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                   const basic_code_node<A>& node, size_t begin = 0);
};

//! Function \c or
/*! Logical disjunction. It uses short-circuit evaluation, that is, arguments
 * are evaluated in order only until the final result is known (until the first
 * \c true or until all values are tested).
 * \param val (0 or more) input values converted to \c bool by
 * f_bool::convert()
 * \return \c true if at least one \a val is \c true; \c false otherwise
 * (including an empty list of arguments)
 * \throw exception::value_null if any evaluated \a val is \c null */
template <impl::allocator A>
class f_or final: public f_or_base<A> {
    using f_or_base<A>::f_or_base;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c or_r
/*! It is similar to f_or, but the first argument is used for storing the
 * result.
 * \param result if it has type \c bool, the result is stored into it;
 * otherwise, a new value is allocated for the result
 * \param val (0 or more) input values converted to \c bool by
 * f_bool::convert()
 * \return \c true if at least one \a val is \c true; \c false otherwise
 * (including an empty list of arguments)
 * \throw exception::op_narg if the number of arguments is 0
 * \throw exception::value_null if any evaluated \a val is \c null */
template <impl::allocator A>
class f_or_r final: public f_or_base<A> {
    using f_or_base<A>::f_or_base;
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

//! Function size
/*! It gets the number of elements of a value.
 * \param result (optional) if it exists and has type \c unsigned, the result
 * is stored into it; otherwise, a new value is allocated for the result
 * \param val a value
 * \return an \c unsigned value: the number of elements of a \c vector or \c
 * hash, the number of characters in a \c string, 1 otherwise (for scalar
 * types)
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a val is \c null */
template <impl::allocator A>
class f_size final: public basic_value_native_fun<f_size<A>, A> {
    using basic_value_native_fun<f_size<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c sub
/*! Numeric subtraction. Unsigned subtraction is done using modulo arithmetic,
 * signed overflow causes exception::op_overflow.
 * \param result (optional) if it exists and has the same type as \a val1 and
 * \a val2, the result is stored into it; otherwise, a new value is allocated
 * for the result
 * \param val1 the first operand, it must be \c int or \c unsigned
 * \param val2 the second operand, it must have the same type as \a val1
 * \return \a val1 - \a val2
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if \a val1 or \a val2 is \c null
 * \throw exception::value_type if \a val1 and \a val2 do not have the same
 * type or if their type is not \c int or \c unsigned
 * \throw exception::op_overflow if \a val1 and \a val2 have type \c int and
 * overflow occurs */
template <impl::allocator A>
class f_sub final: public basic_value_native_fun<f_sub<A>, A> {
    using basic_value_native_fun<f_sub<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c substr
/*! Gets a substring of a string.
 * \param str a substring will be extracted from this string
 * \param idx the index into \a str where the substring begins; if greater or
 * equal to the size of \a str, the returned substring is empty
 * \param (optional) len the length of the substring; if missing or greater
 * than the number of characters from \a idx to the end of \a str, the
 * substring ends at the end of \a str
 * \return the substring
 * \throw exception::op_narg if the number of arguments is not 2 or 3
 * \throw exception::value_null if any argument is \c null
 * \throw exception::value_type if \a str does not have type \a string, or \a
 * idx or \a len is not of type \a int or \a unsigned
 * \throw exception::value_out_of_range if \a idx or \a len is negative */
template <impl::allocator A>
class f_substr final: public basic_value_native_fun<f_substr<A>, A> {
    using basic_value_native_fun<f_substr<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c throw
/*! Throws an exception of type exception::script_throw. This command never
 * returns.
 * \param msg (optional) if used, it must have type \c string, which will be
 * stored in the exception; if missing then the current exception will be
 * thrown
 * \throw - the current exception if called without an argument
 * \throw exception::script_throw containing \a msg if called with an argument
 * \throw exception::op_narg if the number of arguments is not 0 or 1
 * \throw exception::value_null if \a msg is \c null
 * \throw exception::value_type if \a msg does not have type \c string
 * \throw exception::op_bad if called without an argument and there is no
 * current exception */
template <impl::allocator A>
class f_throw final: public basic_value_native_fun<f_throw<A>, A> {
    using basic_value_native_fun<f_throw<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c try
/*! It evaluates an arbitrary command or function and catches exceptions
 * derived from exception::base thrown during evaluation. Other types of C++
 * exceptions are not caught.
 *
 * After argument \a cmd, there are zero or more pairs of arguments \a exc and
 * \a handler. First, \a cmd is evaluated. If it does not throw an exception,
 * its return value is returned. If \a cmd throws an exception, arguments \a
 * exc are compared to the exception sequentially starting from the first one:
 * \arg If \a exc does not begin with character \c '!', it is compared to the
 * exception type name, as returned by exception::base::type().
 * \arg If \a exc begins with \c '!' and the exception type is
 * exception::script_throw, the rest of \a exc value after \c '!' is compared
 * to the exception message, as returned by exception::base::msg().
 * \arg If \a exc is the empty string, it matches any exception.
 * 
 * If a matching \a exc is found, the immediately following argument \a handler
 * is evaluated and its value is returned. If no \a exc matches, the exception
 * is rethrown. An exception thrown from \a handler is propagated and is not
 * caught by the same \c try command.
 * \param cmd a command (or a function) to be evaluated
 * \param exc (repeated 0 or more times) used to match an exception, it must be
 * a \c string
 * \param handle (repeated 0 or more times, once after each \a exc) an
 * exception handler, it may have any type or be \c null
 * \return the return value of \a cmd or of the evaluated \a handler
 * \throw exception::op_narg if called without arguments or with an even number
 * of arguments
 * \throw exception::value_null if matching with argument \a exc having value
 * \c null is done
 * \throw exception::value_type if matching with argument \a exc having other
 * type than \c string is done */
template <impl::allocator A>
class f_try final: public basic_value_native_fun<f_try<A>, A> {
    using basic_value_native_fun<f_try<A>, A>::basic_value_native_fun;
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
 * \throw exception::value_null if \a val is \c null */
template <impl::allocator A>
class f_type final: public basic_value_native_fun<f_type<A>, A> {
    using basic_value_native_fun<f_type<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Function \c unsigned
/*! Converts a value to \c unsigned. An \c unsigned value is returned unchanged.
 * An \c int value is converted using integral conversion rules of C++, that
 * is, using modulo arithmetic. A \c string value is interpreted as a decimal
 * number with optional leading plus sign.
 * \param result (optional) if exists and has type \c int, the result is
 * stored into it; otherwise, a new value is allocated for the result
 * \param val convert this value
 * \return \a val converted to type \c unsigned
 * \throw exception::op_narg if the number of arguments is not 1 or 2
 * \throw exception::value_null if \a val is \c null
 * \throw exception::value_type if \a val is not of type \c int, \c unsigned,
 * or \c string
 * \throw exception::value_bad if string \a val does not contain a decimal
 * number
 * \throw exception::value_out_of_range if a string \a val contains a
 * correctly formated value out of range of type \c unsigned */
template <impl::allocator A>
class f_unsigned final: public basic_value_native_fun<f_unsigned<A>, A> {
    using basic_value_native_fun<f_unsigned<A>, A>::basic_value_native_fun;
protected:
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                            basic_symbol_table<A>& l_vars,
                                            const basic_code_node<A>& node,
                                            std::string_view fun_name) override;
};

//! Command \c var
/*! It gets or sets a \a value of variable \a name. It assings a reference to
 * the \a value instead of copying it. Hence, the same value may be accessible
 * by several names. The variable name is searched first in the symbol table of
 * the current stack frame (the script or the currently executing function;
 * basic_state::stack_frame::l_vars). If not found, it is searched in the
 * global symbol table of the current thread (basic_state::t_vars) and finally
 * in the  symbol table containing global variables shared by all threads of
 * the current virtual machine (basic_virtual_machine::sh_vars). If setting a
 * variable, the variable in the current stack frame is always set.
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

//! Function \c vector
/*! It creates a new empty value of type \c vector.
 * \return the newly created empty \c vector
 * \throw exception::op_narg if the number of arguments is not 0 */
template <impl::allocator A>
class f_vector final: public basic_value_native_fun<f_vector<A>, A> {
    using basic_value_native_fun<f_vector<A>, A>::basic_value_native_fun;
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
 * \return the result of the last evaluation of \a body, or \c null if \a body
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
