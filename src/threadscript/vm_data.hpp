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
 * To be able to mark a value as mt-safe, all other values referenced by it
 * (e.g., by a basic_value_array or basic_value_hash) must be already mt-safe.
 * The reason is that after marked mt-safe, the value may be shared among
 * threads. If a thread accesses a referenced value through the shared value,
 * it must be sure that the referenced value cannot be modified concurrently by
 * another thread.
 * \tparam A the allocator type
 * \threadsafe{safe,unsafe}
 * \test in file test_vm_data.cpp
 * \todo Add member function call() that evaluates the value, if possible,
 * without a reference to basic_code node, which is needed by eval(). It is
 * intended for calling scripts and functions (scripted and native) from native
 * code, outside of a script. */
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
     * basic_value_native_fun), it runs the script or calls the function and
     * returns the result.
     * \param[in] thread the current thread
     * \param[in] lookup the (const) symbol table used for symbol lookups
     * \param[in] sym the (non-const) symbol tables where new  symbols can be
     * added. Usually, \c sym[0] is the same symbol table as \a lookup, which
     * contains the local variables of the current function; \c sym[1] are the
     * global variables of the current thread; \c sym[2], if it exists, is a
     * new symbol table that will eventually become shared by all threads of
     * the virtual machine
     * \param[in] node evaluate in the context of this code node
     * \param[in] fun_name if this evaluation was initiated by a function call,
     * then this is the function name; otherwise, it is empty
     * \return the result of evaluation
     * \throw a class derived from exception::base if evaluation fails; other
     * exceptions are wrapped in exception::wrapped */
    virtual value_ptr eval(basic_state<A>& thread,
        const basic_symbol_table<A>& lookup,
        const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& sym,
        const basic_code_node<A>& node, std::string_view fun_name);
private:
    bool _mt_safe = false; //!< Whether this value is thread-safe
    //! basic_code_node::eval() calls basic_value::eval()
    friend basic_code_node<A>;
};

//! Writes a value to a stream
/*! If \a p is not \c nullptr, then the value is written by its member function
 * basic_value::write(). If \a p is \c nullptr, then the string \c "null" is
 * written.
 * \param[in] os the output stream
 * \param[in] p the value to be written
 * \return \a os */
template <impl::allocator A> std::ostream&
operator<<(std::ostream& os, const typename basic_value<A>::value_ptr& p);

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
        if (v.size() <= v.capacity() / 3)
            v.shrink_to_fit();
        return v;
    }
    //! Writes a textual representation of this value to a stream
    /*! Writes the raw string value, without any escaping
     * \param[in] os the output stream */
    void write(std::ostream& os) const override;
};

template <impl::allocator A> class basic_value_array;

namespace impl {
//! The name of value_array
inline constexpr char name_value_array[] = "array";
//! The base class of basic_value_array
/*!\tparam A an allocator type */
template <allocator A> using basic_value_array_base =
    basic_typed_value<basic_value_array<A>,
        a_basic_vector<typename basic_value<A>::value_ptr, A>,
        name_value_array, A>;
} // namespace impl

//! The value class holding an array of values
/*! \tparam A an allocator type; used internally by the stored array
 * \test in file test_vm_data.cpp */
template <impl::allocator A> class basic_value_array final:
    public impl::basic_value_array_base<A>
{
    static_assert(
            impl::uses_allocator<typename basic_value_array::value_type, A>);
    using impl::basic_value_array_base<A>::basic_value_array_base;
public:
    using impl::basic_value_array_base<A>::value;
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
    typename basic_value_array::value_type& value() {
        typename basic_value_array::value_type& v =
            impl::basic_value_array_base<A>::value();
        if (v.size() <= v.capacity() / 3)
            v.shrink_to_fit();
        return v;
    }
    //! \copybrief impl::basic_value_array_base<A>::set_mt_safe()
    /*! \throw exception::value_mt_unsafe if the array contains at least one
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
        if (v.load_factor() <= v.max_load_factor() / 3)
            v.rehash(v.size() / v.max_load_factor() / 2 * 3);
        return v;
    }
    //! \copybrief impl::basic_value_hash_base<A>::set_mt_safe()
    /*! \throw exception::value_mt_unsafe if the array contains at least one
     * value that is not mt-safe. */
    void set_mt_safe() override;
};

} // namespace threadscript
