#pragma once

/*! \file
 * \brief Data structures of the ThreadScript virtual machine
 *
 * These classes implement various data structures used to store ThreadScript
 * data.
 */

#include "threadscript/allocated.hpp"
#include "threadscript/concepts.hpp"
#include "threadscript/configure.hpp"
#include "threadscript/exception.hpp"
#include "threadscript/template.hpp"

#include <memory>
#include <optional>
#include <typeinfo>

namespace threadscript {

template <impl::allocator A> class basic_state;
template <impl::allocator A> class basic_symbol_table;
template <impl::allocator A> class basic_code_node;

//! The base class for all value types that can be referenced by a symbol_table
/*! It is the base of a hierarchy of polymorphic value classes, therefore it
 * has a virtual destructor. All value types are derived from value_base,
 * except \c null, which is represented by a \c nullptr value of a
 * value_base::ptr.
 *
 * A value can be marked as \e mt-safe (thread-safe). Such value may
 * be accessed by a script running in any thread, but it is read-only, hence
 * any attempt to modify the value throws exception::value_read_only. Being
 * mt-safe is a stronger constraint than being const. The thread-safety flag
 * cannot be unset, because it may not be possible to trace all places that
 * depend on it.
 *
 * For objects, that is, classes derived from basic_value_object, the semantics
 * of mt-safe is defined by an object implementation and may be different from
 * ordinary values. For example, basic_shared_vector and basic_shared_hash are
 * always mt-safe and not read-only, because all operations on them are
 * synchronized by an internal mutex.
 *
 * To be able to mark a value as mt-safe, all other values referenced by it
 * (e.g., by a basic_value_vector or basic_value_hash) must be already mt-safe.
 * The reason is that after marked mt-safe, the value may be shared among
 * threads. If a thread accesses a referenced value through the shared value,
 * it must be sure that the referenced value cannot be modified concurrently by
 * another thread.
 * \tparam A the allocator type
 * \threadsafe{safe,unsafe}
 * \test in file test_vm_data.cpp */
template <impl::allocator A> class basic_value:
    public std::enable_shared_from_this<basic_value<A>> {
public:
    using allocator_type = A; //!< The allocator type used by this class
    //! The shared pointer type to values represented by basic_value objects
    /*! Value \c nullptr is permitted and represents a \c null value. */
    using value_ptr = std::shared_ptr<basic_value>;
    //! The const shared pointer type to a value.
    using const_value_ptr = std::shared_ptr<const basic_value>;
    //! No copying
    basic_value(const basic_value&) = delete;
    //! No moving
    basic_value(basic_value&&) = delete;
    //! No copying
    basic_value& operator=(const basic_value&) = delete;
    //! No moving
    basic_value& operator=(basic_value&&) = delete;
    //! Gets the dynamic type of this value
    /*! \return the type info */
    [[nodiscard]] const std::type_info& type() const noexcept {
        return typeid(*this);
    }
    //! Gets the value type name
    /*! \return the type name */
    [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;
    //! Tests if this value is marked as thread-safe.
    /*! This function just returns the internal mt-safety flag. Any testing if
     * the value may be marked as mt-safe must be done by set_mt_safe().
     * \return the thread-safety flag */
    [[nodiscard]] bool mt_safe() const noexcept { return _mt_safe; }
    //! Sets the thread-safety flag of this value.
    /*! This base class function never throws. Overriding function must call
     * this base class function on success and throw on error.
     * \throw exception::value_mt_unsafe if this value does not satisfy
     * conditions for being thread-safe */
    virtual void set_mt_safe();
    //! Copies this value, but not referenced values.
    /*! It makes a deep copy of the representation of this value itself, but
     * any pointers to other basic_value objects will reference the same
     * objects as in the source object.
     * \param[in] alloc the allocator, whose copy is then passed to any member
     * object that needs an allocator
     * \param[in] mt_safe sets the thread-safety flag of the new value to \c
     * true or \c false; copies the thread-safety flag from the source value if
     * \c nullopt.
     * \return a copy of this value */
    value_ptr shallow_copy(const A& alloc, std::optional<bool> mt_safe = {})
        const
    {
        return shallow_copy_impl(alloc, mt_safe);
    }
    //! Writes a textual representation of this value to a stream
    /*! The default implementation writes the type name.
     * \param[in] os the output stream */
    virtual void write(std::ostream& os) const {
        os << type_name();
    }
protected:
    //! Default constructor
    /*! Protected, because no instances of this class should be created. */
    basic_value() = default;
    //! Virtual default destructor, because this is a polymorphic type
    /*! Protected, because no instances of this class should be created. */
    virtual ~basic_value() = default;
    //! \copydoc basic_value<A>::shallow_copy()
    virtual value_ptr shallow_copy_impl(const A& alloc,
                                        std::optional<bool> mt_safe) const = 0;
    //! Evaluates the value and returns the result.
    /*! For "normal" values, it returns the value itself. For values
     * representing code (basic_value_script, basic_value_function,
     * basic_value_native_fun, basic_value_object,
     * basic_value_object::constructor), it runs the script or calls the
     * function or method and returns the result.
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame;
     * in a function, it contains local variables of the current function;
     * outside of functions, it contains local variable of the current script
     * \param[in] node evaluate in the context of this code node
     * \param[in] fun_name if this evaluation was initiated by a function call,
     * then this is the function name; otherwise, it is empty
     * \return the result of evaluation
     * \throw a class derived from exception::base if evaluation fails; other
     * exceptions are wrapped in exception::wrapped */
    virtual value_ptr eval(basic_state<A>& thread,
        basic_symbol_table<A>& l_vars,
        const basic_code_node<A>& node, std::string_view fun_name);
    //! Gets the number of arguments.
    /*! It is intended to be called from eval().
     * \param[in] node the argument \a node of eval()
     * \return the number of function arguments. */
    size_t narg(const basic_code_node<A>& node) const noexcept;
    //! Evaluates an argument and returns its value.
    /*! It is intended to be called from eval().
     * \param[in] thread the argument \a thread of the caller eval()
     * \param[in] l_vars the argument \a l_vars of the caller eval()
     * \param[in] node the argument \a node of the caller eval()
     * \param[in] idx the (zero-based) index of the argument
     * \return the argument value; \c nullptr if \a idx is greater than the
     * index of the last argument */
    typename basic_value<A>::value_ptr arg(basic_state<A>& thread,
                                           basic_symbol_table<A>& l_vars,
                                           const basic_code_node<A>& node,
                                           size_t idx);
    //! Evaluates an argument as an index and returns its value.
    /*! It is intended to be called from eval() for arguments interpreted as
     * zero-based indices.
     * \param[in] thread the argument \a thread of the caller eval()
     * \param[in] l_vars the argument \a l_vars of the caller eval()
     * \param[in] node the argument \a node of the caller eval()
     * \param[in] idx the (zero-based) index of the argument
     * \return the argument value
     * \throw exception::value_null if the argument is \c null
     * \throw exception::value_type if the argument does not have type \c int
     * or \c unsigned
     * \throw exception::out_of_range if \a idx is negative */
    size_t arg_index(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                     const basic_code_node<A>& node, size_t idx);
    //! Creates a result of a function.
    /*! It is used by implementation classes for commands and function in
     * namespace threadscript::predef.
     * \tparam T a result type, derived from basic_typed_value
     * \param[in] thread the argument \a thread of the caller eval()
     * \param[in] l_vars the argument \a l_vars of the caller eval()
     * \param[in] node the argument \a node of the caller eval()
     * \param[in,out] val the value to be stored in the result; the value will
     * be moved from \a val
     * \param[in] use_arg if \c true and the argument with index \a arg has
     * type \a T, the result is stored in it; otherwise (if \c false or the
     * argument with index \a arg does not have type \a T) a new value is
     * allocated for the result
     * \param[in] arg the index of the output argument
     * \return a basic_typed_value of type \a T, containing \a val
     * \throw exception::value_read_only if the output argument would be used,
     * but it is not writable */
    template <std::derived_from<basic_value<A>> T>
    typename basic_value<A>::value_ptr make_result(basic_state<A>& thread,
                                               basic_symbol_table<A>& l_vars,
                                               const basic_code_node<A>& node,
                                               typename T::value_type&& val,
                                               bool use_arg, size_t arg = 0);
private:
    bool _mt_safe = false; //!< Whether this value is thread-safe
    //! basic_code_node::eval() calls basic_value::eval()
    friend basic_code_node<A>;
};

//! The base class for individual value types.
/*! If type T does some allocations internally, it should use the provided
 * Allocator for this purpose.
 * \tparam Derived the derived value type (it is expected to be \c final)
 * \tparam T the type of the internal stored \ref data
 * \tparam Name the type name used by the ThreadScript engine
 * \tparam A the allocator to be used by \ref data (if needed).
 * \test in file test_vm_data.cpp
 * \note Although a string literal can be used directly as argument \a Name, we
 * define constants instead (e.g., impl::name_value_bool), because the same
 * argument value is used in explicit instantiation in threadscript.hpp and
 * threadscript.cpp. Using constant variable names instead of literals reduces
 * the probability of a typo. */
template <class Derived, class T, str_literal Name, impl::allocator A>
class basic_typed_value: public basic_value<A> {
protected:
    struct tag {}; //!< Used to distiguish between constructors
public:
    //! The type of a stored value
    using value_type = T;
    //! The shared pointer type to \a Derived
    using typed_value_ptr = std::shared_ptr<Derived>;
    //! The shared pointer type to const \a Derived
    using const_typed_value_ptr = std::shared_ptr<const Derived>;
    //! Name of this value type
    /*! \return the type name */
    [[nodiscard]] static consteval std::string_view static_type_name() {
        return Name;
    }
    //! Creates a new default value.
    /*! \param[in] alloc an allocator
     * \return a shared pointer to the created value */
    static typed_value_ptr create(const A& alloc) {
        return std::allocate_shared<Derived>(alloc, tag{}, alloc);
    }
    //! Gets the type name of this value.
    /*! \return a type name */
    [[nodiscard]] std::string_view type_name() const noexcept override final;
    //! Gets read-only access to the contained \ref data.
    /*! \return \ref data */
    const value_type& value() const noexcept { return data; }
    //! Gets read-only access to the contained \ref data.
    /*! \return \ref data */
    const value_type& cvalue() const noexcept { return data; }
    //! Gets writable access to the contained \ref data.
    /*! \return \ref data
     * \throw exception::value_read_only if the value is read-only (that is,
     * marked mt-safe) */
    value_type& value() {
        if (this->mt_safe())
            throw exception::value_read_only();
        return data;
    }
    //! \copydoc basic_value<A>::shallow_copy()
    typed_value_ptr shallow_copy(const A& alloc,
                                 std::optional<bool> mt_safe = {}) const
    {
        return static_pointer_cast<Derived>(shallow_copy_impl(alloc, mt_safe));
    }
    //! Creates a default value.
    /*! \param[in] t an ignored parameter used to overload constructors and to
     * prevent using this constructor directly
     * \param[in] alloc an allocator to be used by \ref data; ignored if \a T
     * does not need an allocator */
    basic_typed_value(tag t, const A& alloc);
protected:
    typename basic_value<A>::value_ptr
        shallow_copy_impl(const A& alloc,
                          std::optional<bool> mt_safe) const override;
private:
    //! Used to distiguish between constructors
    struct tag2 {};
    //! Creates a default value, used if \a T needs an allocator.
    /*! \tparam Alloc an allocator type
     * \param[in] t an ignored parameter used to overload constructors
     * \param[in] a an allocator */
    template <class Alloc> requires impl::uses_allocator<T, Alloc>
    explicit basic_typed_value(tag2 t, const Alloc& a);
    //! Creates a default value, used if \a T does not need an allocator.
    /*! \tparam Alloc an allocator type
     * \param[in] t an ignored parameter used to overload constructors
     * \param[in] a an ignored allocator */
    template <class Alloc> requires (!impl::uses_allocator<T, Alloc>)
    explicit basic_typed_value(tag2 t, const Alloc& a);
    [[no_unique_address]] value_type data; //!< The stored data of this value
};

template <impl::allocator A> class basic_value_bool;

namespace impl {
//! The name of basic_value_bool
inline constexpr char name_value_bool[] = "bool";
//! The base class of basic_value_bool
/*! \tparam A an allocator type */
template <allocator A> using basic_value_bool_base =
    basic_typed_value<basic_value_bool<A>, bool, name_value_bool, A>;
} // namespace impl

//! The value class holding a Boolean value
/*! \tparam A an allocator type; unused
 * \test in file test_vm_data.cpp */
template <impl::allocator A> class basic_value_bool final:
    public impl::basic_value_bool_base<A>
{
    static_assert(!impl::uses_allocator<typename basic_value_bool::value_type,
                  A>);
    using impl::basic_value_bool_base<A>::basic_value_bool_base;
public:
    //! Writes a textual representation of this value to a stream
    /*! Writes the value as string \c "false" or \c "true"
     * \param[in] os the output stream */
    void write(std::ostream& os) const override;
};

template <impl::allocator A> class basic_value_int;

namespace impl {
//! The name of basic_value_int
inline constexpr char name_value_int[] = "int";
//! The base class of basic_value_int
/*! \tparam A an allocator type */
template <allocator A> using basic_value_int_base =
    basic_typed_value<basic_value_int<A>, config::value_int_type,
        name_value_int, A>;
} // namespace impl

//! The value class holding a signed integer value
/*! \tparam A an allocator type; unused
 * \test in file test_vm_data.cpp */
template <impl::allocator A> class basic_value_int final:
    public impl::basic_value_int_base<A>
{
    static_assert(!impl::uses_allocator<typename basic_value_int::value_type,
                  A>);
    using impl::basic_value_int_base<A>::basic_value_int_base;
public:
    //! Writes a textual representation of this value to a stream
    /*! Writes the value as a decimal integer
     * \param[in] os the output stream */
    void write(std::ostream& os) const override;
};

template <impl::allocator A> class basic_value_unsigned;

namespace impl {
//! The name of value_unsigned
inline constexpr char name_value_unsigned[] = "unsigned";
//! The base class of basic_value_unsigned
/*! \tparam A an allocator type */
template <allocator A> using basic_value_unsigned_base =
    basic_typed_value<basic_value_unsigned<A>, config::value_unsigned_type,
        name_value_unsigned, A>;
} // namespace impl

//! The value class holding an unsigned integer value
/*! \tparam A an allocator type; unused
 * \test in file test_vm_data.cpp */
template <impl::allocator A> class basic_value_unsigned final:
    public impl::basic_value_unsigned_base<A>
{
    static_assert(
        !impl::uses_allocator<typename basic_value_unsigned::value_type, A>);
    using impl::basic_value_unsigned_base<A>::basic_value_unsigned_base;
public:
    //! Writes a textual representation of this value to a stream
    /*! Writes the value as a decimal integer
     * \param[in] os the output stream */
    void write(std::ostream& os) const override;
};

template <impl::allocator A> class basic_value_string;

namespace impl {
//! The  name of value_string
inline constexpr char name_value_string[] = "string";
//! The base class of basic_value_string
/*! \tparam A an allocator type */
template <allocator A> using basic_value_string_base =
    basic_typed_value<basic_value_string<A>, a_basic_string<A>,
        name_value_string, A>;
} // namespace impl

//! The value class holding a string value
/*! \tparam A an allocator type; used internally by the stored string
 * value
 * \test in file test_vm_data.cpp */
template <impl::allocator A> class basic_value_string final:
    public impl::basic_value_string_base<A>
{
    static_assert(
              impl::uses_allocator<typename basic_value_string::value_type, A>);
    using impl::basic_value_string_base<A>::basic_value_string_base;
public:
    using impl::basic_value_string_base<A>::value;
    //! Gets writable access to the contained \ref data.
    /*! It handles automatic resizing of storage. Enlarging the capacity is
     * handled by the underlying \c std::string. This function shrinks the
     * capacity if there is a large unused capacity. Common implementations
     * double the capacity when reallocating, therefore we reduce the capacity
     * when only 1/3 of it is used. This prevents frequent reallocations if the
     * size oscillates around the reallocation boundary.
     * \return \ref data
     * \throw exception::value_read_only if the value is read-only (that is,
     * marked thread-safe) */
    typename basic_value_string::value_type& value() {
        typename basic_value_string::value_type& v =
            impl::basic_value_string_base<A>::value();
        std_container_shrink(v);
        return v;
    }
    //! Writes a textual representation of this value to a stream
    /*! Writes the raw string value, without any escaping
     * \param[in] os the output stream */
    void write(std::ostream& os) const override;
};

template <impl::allocator A> class basic_value_vector;

namespace impl {
//! The name of value_vector
inline constexpr char name_value_vector[] = "vector";
//! The base class of basic_value_vector
/*!\tparam A an allocator type */
template <allocator A> using basic_value_vector_base =
    basic_typed_value<basic_value_vector<A>,
        a_basic_vector<typename basic_value<A>::value_ptr, A>,
        name_value_vector, A>;
} // namespace impl

//! The value class holding a vector of values
/*! \tparam A an allocator type; used internally by the stored vector
 * \test in file test_vm_data.cpp */
template <impl::allocator A> class basic_value_vector final:
    public impl::basic_value_vector_base<A>
{
    static_assert(
            impl::uses_allocator<typename basic_value_vector::value_type, A>);
    using impl::basic_value_vector_base<A>::basic_value_vector_base;
public:
    using impl::basic_value_vector_base<A>::value;
    //! Gets writable access to the contained \ref data.
    /*! It handles automatic resizing of storage. Enlarging the capacity is
     * handled by the underlying \c std::vector. This function shrinks the
     * capacity if there is a large unused capacity. Common implementations
     * double the capacity when reallocating, therefore we reduce the capacity
     * when only 1/3 of it is used. This prevents frequent reallocations if the
     * size oscillates around the reallocation boundary.
     * \return \ref data
     * \throw exception::value_read_only if the value is read-only (that is,
     * marked thread-safe) */
    typename basic_value_vector::value_type& value() {
        typename basic_value_vector::value_type& v =
            impl::basic_value_vector_base<A>::value();
        std_container_shrink(v);
        return v;
    }
    //! \copybrief impl::basic_value_vector_base<A>::set_mt_safe()
    /*! \throw exception::value_mt_unsafe if the vector contains at least one
     * value that is not mt-safe. */
    void set_mt_safe() override;
};

template <impl::allocator A> class basic_value_hash;

namespace impl {
//! The name of value_hash
inline constexpr char name_value_hash[] = "hash";
//! The base class of basic_value_hash
/*! \tparam A an allocator type */
template <allocator A> using basic_value_hash_base =
    basic_typed_value<basic_value_hash<A>,
        a_basic_hash<a_basic_string<A>, typename basic_value<A>::value_ptr, A>,
        name_value_hash, A>;
} // namespace impl

//! The value class holding values hashed by string keys
/*! \tparam A an allocator type; used internally by the stored
 * unordered map.
 * \test in file test_vm_data.cpp */
template <impl::allocator A> class basic_value_hash final:
    public impl::basic_value_hash_base<A>
{
    static_assert(
            impl::uses_allocator<typename basic_value_hash::value_type, A>);
    using impl::basic_value_hash_base<A>::basic_value_hash_base;
public:
    using impl::basic_value_hash_base<A>::value;
    //! Gets writable access to the contained \ref data.
    /*! It handles automatic resizing of storage. Reducing the load factor is
     * handled by the underlying \c std::unordered_hash. This function rehashes
     * if the load factor decreases significantly. Rehashing increases the load
     * factor to a value less than the configured maximum. This prevents
     * frequent rehashing if the size oscillates around the maximum load factor
     * boundary.
     * \return \ref data
     * \throw exception::value_read_only if the value is read-only (that is,
     * marked thread-safe) */
    typename basic_value_hash::value_type& value() {
        typename basic_value_hash::value_type& v =
            impl::basic_value_hash_base<A>::value();
        std_container_shrink(v);
        return v;
    }
    //! \copybrief impl::basic_value_hash_base<A>::set_mt_safe()
    /*! \throw exception::value_mt_unsafe if the vector contains at least one
     * value that is not mt-safe. */
    void set_mt_safe() override;
};

//! The base class for objects of native classes implemented in C++
/*! Each class \a Object derived from an instance of this template represents a
 * ThreadScript class with native C++ implementation. See the test program
 * test_object.cpp for examples how to define native classes.
 *
 * In order to use objects of this class in a script, the ThreadScript
 * initialization code (in native C++) creates an instance of class \ref
 * constructor and makes it available via a basic_symbol_table. Each evaluation
 * of \ref constructor creates a new instance of \a Object.
 *
 * The script can call methods of the object by evaluating the object and
 * passing the method name as the first argument.
 * \tparam Object a native class, which must be derived from basic_value_object
 * (using CRTP); it should be \c final, because objects of classes derived from
 * \a Object cannot be created by \ref constructor
 * \tparam Name the name of the class (visible in ThreadScript)
 * \tparam A an allocator type
 * \test in file test_object.cpp */
template <class Object, str_literal Name, impl::allocator A>
class basic_value_object: public basic_value<A> {
protected:
    //! Used to control access to public constructors
    struct tag{};
public:
    //! This base class of \a Object.
    /*! It is especially useful for a definition of inheriting constructor in a
     * derived class as: <tt>using base::base;</tt> */
    using base = basic_value_object<Object, Name, A>;
    using typename basic_value<A>::value_ptr;
    //! Implementation of an object method
    /*! It is called by eval(). Parameters are passed from eval(). A return
     * value is returned by eval(). A thrown exception is thrown by eval(). */
    using method_impl = value_ptr (Object::*)(basic_state<A>& thread,
                                              basic_symbol_table<A>& l_vars,
                                              const basic_code_node<A>& node);
    //! Mapping from method names to implementations
    using method_table = a_basic_hash<a_basic_string<A>, method_impl, A>;
    //! The class of an object used to create instances of \a Object
    class constructor: public basic_value<A> {
    public:
        //! Creates the constructor object
        /*! \param[in] t an ignored parameter that prevents using this
         * constructor directly
         * \param[in] alloc an allocator used to create \ref methods */
        constructor(tag t, const A& alloc);
        //! Gets the value type name
        /*! \return \c "constructor" for a constructor of any \a Object type */
        std::string_view type_name() const noexcept override;
        //! Creates the constructor object
        /*! \param[in] alloc an allocator used to create \ref methods
         * \return the created constructor */
        static value_ptr create(const A& alloc);
    protected:
        //! Creates an instance of \a Object
        /*! \copydetails basic_value::eval() */
        value_ptr eval(basic_state<A>& thread,
                       basic_symbol_table<A>& l_vars,
                       const basic_code_node<A>& node,
                       std::string_view fun_name) final override;
        /*! \copydoc threadscript::basic_value::shallow_copy()
         * \throw exception::not_implemented is always thrown, because \ref
         * constructor is not copyable */
        value_ptr shallow_copy_impl(const A& alloc, std::optional<bool> mt_safe)
            const override;
    private:
        //! The name of this class, as returned by type_name()
        inline static constexpr
            std::string_view constructor_type{"constructor"};
        //! The table of methods for \a Object instances
        std::shared_ptr<const method_table> methods;
    };
    //! Creates the object
    /*! Parameters after \a t are passed by constructor::eval().
     * \param[in] t an ignored parameter that prevents using this
     * \param[in] methods the mapping from method names to implementations
     * \param[in] thread the current thread
     * \param[in] l_vars the symbol table of the current stack frame;
     * in a function, it contains local variables of the current function;
     * outside of functions, it contains local variable of the current script
     * \param[in] node evaluate in the context of this code node
     * \throw a class derived from exception::base if evaluation fails; other
     * exceptions are wrapped in exception::wrapped */
    basic_value_object(tag t, std::shared_ptr<const method_table> methods,
                       basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                       const basic_code_node<A>& node);
    //! Name of this value type
    /*! \return \a Name */
    [[nodiscard]] static consteval std::string_view static_type_name() {
        return Name;
    }
    //! Gets the value type name
    /*! \return \a Name */
    [[nodiscard]] std::string_view type_name() const noexcept final override;
    //! Gets the mapping from method names to implementations
    /*! \return the table used by basic_value_object::constructor and stored in
     * \ref methods
     * \note This function is public, because otherwise \ref base would have to
     * be declaread as \c friend in each derived \a Object class. */
    [[nodiscard]] static method_table init_methods();
    //! Creates basic_value_object::constructor, registers it in a symbol table
    /*! The \ref constructor variable name will be \a Name.
     * \param[in] sym registers the class in this symbol table
     * \param[in] replace if \c false, any existing symbol with the name equal
     * to \a Name is left unchanged; if \c true, any such symbol is replaced by
     * the created constructor value */
    static void register_constructor(basic_symbol_table<A>& sym, bool replace);
protected:
    //! Gets the object or calls a method of the object.
    /*! The method name is passed as the first argument,
     * <tt>arg(thread, l_vars, node, 0)</tt>. If called without arguments, the
     * object itself is returned. If called with a method name, the method
     * result is returned. It calls the method implementation member function
     * (of type method_impl) according to the table returned by init_methods().
     * The implementing member functions gets all arguments of the method call,
     * including the method name in the first argument.
     *
     * \copydetails basic_value::eval()
     * \throw exception::value_null if the first argument (method name) is \c
     * null
     * \throw exception::value_type if the first argument (method name) does
     * not have type \c string
     * \throw exception::not_implemented if the method name is unknown */
    value_ptr eval(basic_state<A>& thread,
                   basic_symbol_table<A>& l_vars,
                   const basic_code_node<A>& node,
                   std::string_view fun_name) override;
    /*! \copydoc threadscript::basic_value::shallow_copy()
     * \throw exception::not_implemented is always thrown, because \a Object is
     * not copyable by default */
    value_ptr shallow_copy_impl(const A& alloc, std::optional<bool> mt_safe)
        const override;
private:
    //! The table of methods
    /*! It is initialized by \ref constructor, which obtains it by calling
     * init_methods(). */
    std::shared_ptr<const method_table> methods;
};

} // namespace threadscript
