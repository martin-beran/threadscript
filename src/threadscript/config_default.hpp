#pragma once

/*! \file
 * \brief Source and build configuration file
 *
 * This file contains default variants of various declarations configurable at
 * build time. To modify some of these declarations, copy this file under a new
 * name, edit it as needed, and set CMake variable \c CONFIG_FILE_HPP to the
 * name of the new file.
 *
 * <b>This file must not be included directly.</b> Instead, use
 * \code
 * #include "threadscript/configure.hpp"
 * \endcode
 * This includes the correct configuration file as selected by CMake.
 */

#ifndef THREADSCRIPT_INCLUDED_BY_CONFIGURE_HPP
#error This file may be included only by threadscript/configure.hpp
#endif

#include <cstddef>
#include <cstdint>

//! The namespace for source configuration declarations.
/*! It contains various declarations configurable at build time. */
namespace threadscript::config {

//! The type used for various counters.
using counter_type = uint64_t;

//! The type used for unsigned integer values
using value_int_type = int64_t;

//! The type used for signed integer values
using value_unsigned_type = uint64_t;

} // namespace threadscript::config

namespace threadscript {

class allocator_config;

template <class T, class CfgPtr> class default_allocator;

namespace config {

//! The type used as the allocator.
template <class T> using allocator_type =
    threadscript::default_allocator<T, allocator_config*>;

} // namespace config

} // namespace threadscript

#include "threadscript/default_allocator.hpp"
