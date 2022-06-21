#pragma once

/*! \file
 * \brief Selection of a build configuration file
 *
 * This file includes the generated configuration file threadscript/config.hpp,
 * and \b must be used instead of directly including threadscript/config.hpp or
 * threadscript/config_default.hpp. It handles selection between
 * threadscript/config.hpp.in (when running Doxygen) and
 * threadscript/config.hpp (when running the compiler).
 */

#define THREADSCRIPT_INCLUDED_BY_CONFIGURE_HPP

#ifdef DOXYGEN
#include "threadscript/config.hpp.in"
#else
#include "threadscript/config.hpp"
#endif

#undef THREADSCRIPT_INCLUDED_BY_CONFIGURE_HPP
