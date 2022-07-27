#pragma once

/*! \file
 * \brief The main public interface to ThreadScript parser.
 */

#include "threadscript/code.hpp"
#include "threadscript/syntax.hpp"

namespace threadscript {

//! The iterator type used by parse_code() and parse_code_file().
using parse_iterator = parser::script_iterator<std::string_view::iterator>;

//! The exception throws when parsing fails.
using parse_error = parser::error<parse_iterator>;

//! Parses a script source text
/*! The \a file is not accessed during parsing. It is expected that its
 * content is provided in \a src. The file name is only used for storing and
 * reporting code locations.
 * \tparam A the allocator type
 * \param[in] alloc the allocator used to allocate the returned parsed script
 * \param[in] src the source code to be parsed
 * \param[in] file a file name, which will be stored in the internal
 * representation of the script
 * \param[in] syntax the syntax variant of the script
 * \param[in] trace an optional tracing function
 * \return the internal representation of the parsed script; never \c nullptr
 * \throw exception::parse_error if parsing fails */
template <impl::allocator A> basic_script<A>::script_ptr
parse_code(const A& alloc, std::string_view src, std::string_view file,
           std::string_view syntax = syntax_factory::syntax_canon,
           parser::context::trace_t trace = {});

//! Parses a script file
/*! \tparam A the allocator type
 * \param[in] alloc the allocator used to allocate the returned parsed script
 * \param[in] file the file name
 * \param[in] syntax the syntax variant of the script
 * \param[in] trace an optional tracing function
 * \return the internal representation of the parsed script; never \c nullptr
 * \throw exception::parse_error if parsing fails */
template <impl::allocator A> basic_script<A>::script_ptr
parse_code_file(const A& alloc, std::string_view file,
                std::string_view syntax = syntax_factory::syntax_canon,
                parser::context::trace_t trace = {});

} // namespace threadscript
