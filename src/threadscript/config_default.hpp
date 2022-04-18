#pragma once

/*! \file
 * \brief Source and build configuration file
 *
 * This file contains default variants of various declarations configurable at
 * build time. To modify some of these declarations, copy this file under a new
 * name, edit it as needed, and set CMake variable \c CONFIG_FILE_HPP to the
 * name of the new file.
 *
 * This file must not be included directly. Instead, use
 * \code
 * #include "threadscript/config.hpp"
 * \endcode
 * This includes the correct configuration file as selected by CMake.
 */

#include <cstddef>
#include <cstdint>

//! The namespace for source configuration declarations.
/*! It contains various declarations configurable at build time. */
namespace threadscript::config {

//! The type used to store sizes of various objects.
using size_type = std::size_t;
//! The type used for various counters.
using counter_type = std::uint64_t;

} // namespace threadscript::config

#include "threadscript/default_allocator.hpp"

namespace threadscript::config {

//! The type used as the allocator.
template <class T> using allocator_type =
    threadscript::default_allocator<T, allocator_config*>;

} // namespace threadscript::config
