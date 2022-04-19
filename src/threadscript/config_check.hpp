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
#include "threadscript/config.hpp"

#include <type_traits>

namespace threadscript::config {

static_assert(std::is_unsigned_v<counter_type>);
static_assert(std::is_unsigned_v<value_unsigned_type>);
static_assert(std::is_signed_v<value_int_type>);

} // namespace threadscript::config
