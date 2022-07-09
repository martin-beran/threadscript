#pragma once

/*! \file
 * \brief A generic parser for ASCII input
 *
 * \test in file test_parser_ascii.cpp
 */

#include "threadscript/parser.hpp"

//! The namespace containing a generic parser for ASCII input
/*! It contains specializations of templates from threadscript::parser for
 * parsing sequences of \c char values in ASCII encoding and usually accessed
 * via threadscript::parser::script_iterator. It allows 8 bit values (0--255
 * or -128--127, depending on signedness of \c char), not only 7 bit ASCII
 * characters (0--127). It does not provide any special support for parsing
 * Unicode UTF-8 data. The handler type is fixed to
 * threadscript::parser::default_handler.
 * \test in file test_parser_ascii.cpp */
namespace threadscript::parser_ascii {

//! An iterator type used for parsing ASCII data
/*! \tparam It an iterator type */
template <class It>
concept ascii_iterator = std::forward_iterator<It> &&
    std::same_as<typename std::iterator_traits<It>::value_type, char>;

//! A predicate to test a \c char value
/*! \tparam Predicate a predicate
 * \tparam It an iterator to the input sequence of \c char */
template <class Predicate, class It>
concept predicate = parser::predicate<Predicate, It> && ascii_iterator<It>;

//! A predicate to compare two \c char values
/*! \tparam Predicate a predicate
 * \tparam It an iterator to the input sequence of \c char */
template <class Predicate, class It>
concept predicate2 = parser::predicate2<Predicate, It> && ascii_iterator<It>;

//! A handler called when a rule matches
/*! \tparam Handler a handler
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of a rule
 * \tparam Up a temporary context of a parent rule (will be \c nullptr if a
 * rule has no parent)
 * \tparam Info information about a parsed part of input; it is expected to be
 * \ref threadscript::parser::empty unless specified otherwise for a particular
 * rule type
 * \tparam It an iterator to the input sequence of terminal symbols */
template <class Ctx, class Self, class Up, class Info, std::forward_iterator It>
    requires ascii_iterator<It>
using handler = parser::default_handler<Ctx, Self, Up, Info, It>;

//! This namespace contains various reusable parser rules
/*! There are some rules frequently needed by parsers of textual data, for
 * example, rules for end-of-line, whitespace, numbers, or identifiers.
 * \test in file test_parser_ascii.cpp */
namespace rules {
} // namespace rules

} // namespace threadscript::parser_ascii
