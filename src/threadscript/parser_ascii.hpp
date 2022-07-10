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

//! Converts a character to lowercase.
/*! It assumes ASCII, that is, letters A--Z and a--z, not depending on the
 * current locale.
 * \param[in] c a character
 * \return an uppercase \a c converted to lowercase, otherwise \a c unchanged */
[[nodiscard]] inline char to_lower(char c) noexcept
{
    return char(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
}

//! Converts a character to uppercase.
/*! It assumes ASCII, that is, letters A--Z and a--z, not depending on the
 * current locale.
 * \param[in] c a character
 * \return an uppercase \a c converted to lowercase, otherwise \a c unchanged */
[[nodiscard]] inline char to_upper(char c) noexcept
{
    return char(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
}

//! A function object that performs case-insensitive comparison of characters.
/*! It treats corresponding characters from ASCII uppercase (A--Z) and
 * lowercase (a--z) as equal. It does not depend on the current locale. */
class equal_ic {
public:
    //! Tests characters \a and \b for case-insensitive equality.
    /*! \param[in] a the first character
     * \param[in] b the second character
     * \return if \a a and \a b are equal */
    bool operator()(char a, char b) const noexcept {
        return to_lower(a) == to_lower(b);
    }
};

//! This namespace contains various reusable parser rules
/*! There are some rules frequently needed by parsers of textual data, for
 * example, rules for end-of-line, whitespace, numbers, or identifiers.
 * \test in file test_parser_ascii.cpp */
namespace rules {

//! Creates rules with a common set of template parameters.
/*! This template provides an easy way to specify set of template parameters
 * that cannot be deduced from arguments of rule creation functions. An alias
 * can be defined for an instance of this template and then used for creating
 * individual rules. See test_parser_ascii.cpp for examples how to use this
 * class.
 * \tparam It an iterator to the input sequence
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule */
template <ascii_iterator It, class Ctx = parser::context,
    class Self = parser::empty, class Up = parser::empty>
class factory {
public:
    //! Creates parser::rules::fail
    /*! \return the created rule */
    static auto fail() {
        return parser::rules::fail<Ctx, Self, Up, It,
            handler<Ctx, Self, Up, parser::empty, It>>{};
    }
    //! Creates parser::rules::eof
    /*! This rule does not consume any input, therefore it must not be used in
     * an unlimited rules::repeat, because it would create an endless loop.
     * \return the created rule */
    static auto eof() {
        return parser::rules::eof<Ctx, Self, Up, It,
            handler<Ctx, Self, Up, parser::empty, It>>{};
    }
    //! Creates parser::rules::any
    /*! \return the created rule */
    static auto any() {
        return parser::rules::any<Ctx, Self, Up, It,
            handler<Ctx, Self, Up, parser::empty, It>>{};
    }
    //! Creates parser::rules::t
    /*! \param[in] c the matching character
     * \return the created rule */
    static auto t(char c) {
        return parser::rules::t<Ctx, Self, Up, It,
            handler<Ctx, Self, Up, parser::empty, It>>{c};
    }
    //! Creates parser::rules::p
    /*! \tparam Predicate a predicate for testing if a symbol matches
     * \param[in] pred the predicate for testing input
     * \return the created rule */
    template <predicate<It> Predicate>
    static auto p(Predicate pred) {
        return parser::rules::p<Ctx, Self, Up, It, Predicate,
            handler<Ctx, Self, Up, parser::empty, It>>{std::move(pred)};
    }
    //! Creates parser::rules::str
    /*! It tests that a predicate returns \c true for corresponding \c char
     * from the input and from the stored \c std::string.
     * \tparam Predicate a predicate for testing if a \c char matches; by
     * default, an equality test is performed, a common alternative is
     * case-insensitive matching by equal_ic.
     * \param[in] seq a string to be matched; stored in the created rule object
     * \param[in] pred the predicate for testing characters
     * \return the created rule */
    template <predicate2<It> Predicate = std::equal_to<char>>
    static auto str(std::string seq, Predicate pred = {}) {
        return parser::rules::str<Ctx, Self, Up, It, std::string, Predicate,
            handler<Ctx, Self, Up, parser::empty, It>>{std::move(seq),
                std::move(pred)};
    }
    //! Creates parser::rules::str
    /*! \param[in] seq a string to be matched; stored in the created rule object
     * \return the created rule */
    static auto str_ic(std::string seq) {
        return parser::rules::str<Ctx, Self, Up, It, std::string, equal_ic,
            handler<Ctx, Self, Up, parser::empty, It>>{std::move(seq),
                equal_ic()};
    }
    //! Creates parser::rules::str
    /*! It tests that a predicate returns \c true for corresponding \c char from
     * the input and from the stored \c std::string_view.
     * \tparam Predicate a predicate for testing if a \c char matches; by
     * default, an equality test is performed, a common alternative is
     * case-insensitive matching by equal_ic.
     * \param[in] seq a string view to be matched; the created rule object
     * contains only a reference to an externally stored sequences of
     * characters, which must not be destroyed during the lifetime of the
     * created rule
     * \param[in] pred the predicate for testing characters
     * \return the created rule */
    template <predicate2<It> Predicate = std::equal_to<char>>
    static auto str(std::string_view seq, Predicate pred = {}) {
        return parser::rules::str<Ctx, Self, Up, It,
            std::string_view, Predicate,
            handler<Ctx, Self, Up, parser::empty, It>>{seq, std::move(pred)};
    }
    //! Creates parser::rules::str
    /*! \param[in] seq a string view to be matched; the created rule object
     * contains only a reference to an externally stored sequences of
     * characters, which must not be destroyed during the lifetime of the
     * created rule
     * \return the created rule */
    static auto str_ic(std::string_view seq) {
        return parser::rules::str<Ctx, Self, Up, It, std::string_view, equal_ic,
            handler<Ctx, Self, Up, parser::empty, It>>{seq, equal_ic()};
    }
};

} // namespace rules

} // namespace threadscript::parser_ascii
