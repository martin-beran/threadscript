#pragma once

/*! \file
 * \brief Data structures of the ThreadScript virtual machine
 *
 * These classes implement various data structures used to store ThreadScript
 * data and execution state, e.g., values, variables, symbol tables, or stacks.
 */

#include "threadscript/config.hpp"
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
 * \threadsafe{safe,unsafe} */
template <class Allocator> class basic_value:
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
     * \throw exception::value_mt_unsafe if setting as this value does not
     * satisfy conditions for being thread-safe */
    virtual void set_mt_safe() { _mt_safe = true; }
protected:
    //! Default constructor
    /*! Protected, because no instances of this class should be created. */
    basic_value() = default;
    //! Virtual default destructor
    /*! Protected, because no instances of this class should be created. */
    virtual ~basic_value() = default;
    //! Copies this value, but not referenced values.
    /*! It makes a deep copy of the representation of this value itself, but
     * any pointers to other basic_value objects will reference the same
     * objects as in the source object.
     * \param[in] alloc the allocator used to make the copy and passed to any
     * member object that needs an allocator
     * \return a copy of this value */
    value_ptr shallow_copy(const Allocator& alloc) const;
private:
    bool _mt_safe = false; //!< Whether this value is thread-safe
};

//! The base class for individual value types.
/*! \tparam Derived the derived value type (it is expected to be \c final)
 * \tparam T the type of the internal stored \ref data
 * \tparam Name the type name used by the ThreadScript engine
 * \tparam Allocator the allocator to be used by \ref data (if needed). */
template <class Derived, class T, const char* Name, class Allocator>
class basic_typed_value: public basic_value<Allocator> {
    struct tag {}; //!< Used to distiguish between constructors
protected:
    explicit basic_typed_value(const Allocator& alloc);
private:

    template <class A>
    basic_typed_value(tag, std::enable_if<uses_allocator<T>, A> alloc):
        T{alloc} {}
    template <class A>
    basic_typed_value(std::enable_if<!uses_allocator<T>, tag>): T{} {}
    T data; //! The stored data of this value
};

} // namespace threadscript
