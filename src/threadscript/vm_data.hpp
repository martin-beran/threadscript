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

class value;

//! The shared pointer type used for values represented by \ref value objects
/*! Value \c nullptr is permitted and represents a \c null value. */
using value_ptr = std::shared_ptr<value>;

//! The base class for all value types that can be referenced by a symbol table
/*! It is the base of a hierarchy of polymorphic value classes, therefore it
 * has a virtual destructor. All value types are derived from \ref value,
 * except \c null, which is represented by a \c nullptr value of a value_ptr.
 * \threadsafe{safe,unsafe} */
class value: public std::enable_shared_from_this<value> {
public:
    //! Default constructor
    value() = default;
    //! No copying
    value(const value&) = delete;
    //! No moving
    value(value&&) = delete;
    //! Virtual default destructor
    virtual ~value() = default;
    //! No copying
    value& operator=(const value&) = delete;
    //! No moving
    value& operator=(value&&) = delete;
};

} // namespace threadscript
