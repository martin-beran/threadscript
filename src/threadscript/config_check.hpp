#pragma once

/*! \file
 * \brief Compile time tests of definitions of compile time configuration.
 *
 * It checks declarations in the compile time source code configuration file,
 * which is selected by CMake and \c threadscript/config.hpp.
 *
 * This file is included in \c threadscript/config.hpp and must not be included
 * directly in other files.
 */

// This include is ignored in a normal build, because this file is included by
// threadscript/config.hpp. It is here only to provide configuration
// declarations for an editor.
#include "threadscript/configure.hpp"

#include <limits>
#include <type_traits>

namespace threadscript::config {

static_assert(std::is_unsigned_v<counter_type>);
static_assert(std::is_unsigned_v<value_unsigned_type>);
static_assert(std::is_signed_v<value_int_type>);
// Same number of bits needed for correct artithmetic and conversions
static_assert(std::numeric_limits<value_unsigned_type>::digits ==
              std::numeric_limits<value_int_type>::digits + 1);
static_assert(sizeof(config::value_unsigned_type) ==
              sizeof(config::value_int_type));
// This always holds by definition of intmax_t and uintmax_t
static_assert(sizeof(config::value_unsigned_type) <= sizeof(uintmax_t));
static_assert(sizeof(config::value_int_type) <= sizeof(intmax_t));
// This is required, e.g., for conversion from a std::string by std::stoull or
// std::stoll
static_assert(sizeof(config::value_unsigned_type) <=
              sizeof(unsigned long long));
static_assert(sizeof(config::value_int_type) <= sizeof(long long));
// This is required, e.g., for correct handling of string or container sizes
static_assert(sizeof(config::value_unsigned_type) <= sizeof(size_t));

} // namespace threadscript::config
