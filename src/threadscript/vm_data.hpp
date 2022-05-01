#pragma once

/*! \file
 * \brief Data structures of the ThreadScript virtual machine
 *
 * These classes implement various data structures used to store ThreadScript
 * data and execution state, e.g., values, variables, symbol tables, or stacks.
 */

#include "threadscript/concepts.hpp"
#include "threadscript/config.hpp"
#include "threadscript/config_default.hpp"
#include "threadscript/exception.hpp"
#include <memory>

namespace threadscript {

//! The base class for all value types that can be referenced by a symbol table
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
 * \tparam Allcator the allocator type
 * \threadsafe{safe,unsafe}
 * \test in file test_vm_data.cpp */
template <impl::allocator Allocator> class basic_value:
    public std::enable_shared_from_this<basic_value<Allocator>> {
public:
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
    //! Gets the value type name
    /*! \return the type name */
    [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;
    //! Tests if this value is marked as thread-safe.
    /*! \return the thread-safety flag */
    [[nodiscard]] bool mt_safe() const noexcept { return _mt_safe; }
    //! Sets the thread-safety flag of this value.
    /*! This base class function never throws. Overriding function must call
     * this base class function on success and throw on error.
     * \throw exception::value_mt_unsafe if this value does not satisfy
     * conditions for being thread-safe */
    virtual void set_mt_safe();
protected:
    //! Default constructor
    /*! Protected, because no instances of this class should be created. */
    basic_value() = default;
    //! Virtual default destructor, because this is a polymorphic type
    /*! Protected, because no instances of this class should be created. */
    virtual ~basic_value() = default;
    //! Copies this value, but not referenced values.
    /*! It makes a deep copy of the representation of this value itself, but
     * any pointers to other basic_value objects will reference the same
     * objects as in the source object.
     * \param[in] alloc the allocator used to make the copy and passed to any
     * member object that needs an allocator
     * \return a copy of this value */
    virtual value_ptr shallow_copy_impl(const Allocator& alloc) const = 0;
private:
    bool _mt_safe = false; //!< Whether this value is thread-safe
};

//! The base class for individual value types.
/*! If type T does some allocations internally, it should use the provided
 * Allocator for this purpose.
 * \tparam Derived the derived value type (it is expected to be \c final)
 * \tparam T the type of the internal stored \ref data
 * \tparam Name the type name used by the ThreadScript engine
 * \tparam Allocator the allocator to be used by \ref data (if needed).
 * \test in file test_vm_data.cpp */
template <class Derived, class T, const char* Name, impl::allocator Allocator>
class basic_typed_value: public basic_value<Allocator> {
protected:
    struct tag {}; //!< Used to distiguish between constructors
public:
    //! The shared pointer type to \a Derived
    using typed_value_ptr = std::shared_ptr<Derived>;
    //! The shared pointer type to const \a Derived
    using const_typed_value_ptr = std::shared_ptr<const Derived>;
    //! Gets the type name of this value.
    /*! \return a type name */
    [[nodiscard]] std::string_view type_name() const noexcept override final;
    //! Gets read-only access to the contained \ref data.
    /*! \return \ref data */
    const T& value() const noexcept { return data; }
    //! Gets writable access to the contained \ref data.
    /*! \return \ref data
     * \throw exception::value_read_only if the value is read-only (that is,
     * marked thread-safe) */
    T& value() {
        if (this->mt_safe())
            throw exception::value_mt_unsafe();
        return data;
    }
    //! Copies this value, but not referenced values.
    /*! It makes a deep copy of the representation of this value itself, but
     * any pointers to other basic_value objects will reference the same
     * objects as in the source object.
     * \param[in] alloc the allocator used to make the copy and passed to any
     * member object that needs an allocator
     * \return a copy of this value */
    typed_value_ptr shallow_copy(const Allocator& alloc) const {
        static_pointer_cast<typed_value_ptr>(shallow_copy_impl(alloc));
    }
    //! Creates a default value.
    /*! \param[in] t an ignored parameter used to overload constructors
     * \param[in] alloc an allocator to be used by \ref data; ignored if \a T
     * does not need an allocator */
    explicit basic_typed_value([[maybe_unused]] tag t,
                               const Allocator& alloc):
        basic_typed_value(alloc) {}
protected:
    typename basic_value<Allocator>::value_ptr
        shallow_copy_impl(const Allocator& alloc) const override;
private:
    //! Creates a default value, used if \a T needs an allocator.
    /*! \param[in] a an allocator */
    template <class A> requires impl::uses_allocator<T, A>
    explicit basic_typed_value(const A& a);
    //! Creates a default value, used if \a T does not need an allocator.
    /*! \param[in] a an ignored allocator */
    template <class A> requires (!impl::uses_allocator<T, A>)
    explicit basic_typed_value(const A& a);
    T data; //! The stored data of this value
};

template <impl::allocator Allocator> class basic_value_bool;

namespace impl {
//! The name of basic_value_bool
inline constexpr char name_value_bool[] = "bool";
//! The base class of basic_value_bool
/*! \tparam Allocator an allocator type */
template <allocator Allocator> using basic_value_bool_base =
    basic_typed_value<basic_value_bool<Allocator>, bool, name_value_bool,
        Allocator>;
} // namespace impl

//! The value class holding a Boolean value
/*! \tparam Allocator an allocator type; unused
 * \test in file test_vm_data.cpp */
template <impl::allocator Allocator> class basic_value_bool final:
    public impl::basic_value_bool_base<Allocator>
{
    using impl::basic_value_bool_base<Allocator>::basic_typed_value;
};

template <impl::allocator Allocator> class basic_value_int;

namespace impl {
//! The name of basic_value_int
inline constexpr char name_value_int[] = "int";
//! The base class of basic_value_int
/*! \tparam Allocator an allocator type */
template <allocator Allocator> using basic_value_int_base =
    basic_typed_value<basic_value_int<Allocator>, config::value_int_type,
        name_value_int, Allocator>;
} // namespace impl

//! The value class holding a signed integer value
/*! \tparam Allocator an allocator type; unused
 * \test in file test_vm_data.cpp */
template <impl::allocator Allocator> class basic_value_int final:
    public impl::basic_value_int_base<Allocator>
{
    using impl::basic_value_int_base<Allocator>::basic_typed_value;
};

template <impl::allocator Allocator> class basic_value_unsigned;

namespace impl {
//! The name of value_unsigned
inline constexpr char name_value_unsigned[] = "unsigned";
//! The base class of basic_value_unsigned
/*! \tparam Allocator an allocator type */
template <allocator Allocator> using basic_value_unsigned_base =
    basic_typed_value<basic_value_unsigned<Allocator>,
        config::value_unsigned_type, name_value_unsigned, Allocator>;
} // namespace impl

//! The value class holding an unsigned integer value
/*! \tparam Allocator an allocator type; unused
 * \test in file test_vm_data.cpp */
template <impl::allocator Allocator> class basic_value_unsigned final:
    public impl::basic_value_unsigned_base<Allocator>
{
    using impl::basic_value_unsigned_base<Allocator>::basic_typed_value;
};

template <impl::allocator Allocator> class basic_value_string;

namespace impl {
//! The  name of value_string
inline constexpr char name_value_string[] = "string";
//! The base class of basic_value_string
/*! \tparam Allocator an allocator type */
template <allocator Allocator> using basic_value_string_base =
    basic_typed_value<basic_value_string<Allocator>, std::string,
        name_value_string, Allocator>;
} // namespace impl

//! The value class holding a string value
/*! \tparam Allocator and allocator type; used internally by the stored string
 * value
 * \test in file test_vm_data.cpp */
template <impl::allocator Allocator> class basic_value_string final:
    public impl::basic_value_string_base<Allocator>
{
    using impl::basic_value_string_base<Allocator>::basic_typed_value;
};

} // namespace threadscript
