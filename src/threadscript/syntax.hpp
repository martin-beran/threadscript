#pragma once

/*! \file
 * \brief Common interface to parsers for syntax variants
 *
 * This file contains a base class for parsers of ThreadScript syntax variants.
 * It also provides an interface for parser selection and invoking the selected
 * parser on a source file.
 */

#include "threadscript/parser_ascii.hpp"

#include <map>
#include <memory>
#include <ranges>
#include <string_view>

namespace threadscript {

//! The namespace containing parsers for variants of ThreadScript syntax
namespace syntax {
} // namespace syntax

class script_builder;

//! The base class of parsers declared in namespace threadscript::syntax
/*! It is an abstract class.
 * \note A parser usually uses the PImpl idiom for implementation (see
 * syntax::canon for an example). It allows to keep a syntax header file (e.g.,
 * threadscript/syntax_common.hpp), which is included in syntax.cpp, short and
 * hide the rules of the grammar in the implementation \c .cpp file (e.g.,
 * syntax_common.cpp). */
class syntax_base {
public:
    //! Iterator type used by the parser
    using iterator_type =
        parser::script_iterator<std::string_view::iterator>;
    //! The rule factory used by the parser
    using rf = parser_ascii::rules::factory<iterator_type>;
    //! The default constructor
    syntax_base() = default;
    //! No copy
    syntax_base(const syntax_base&) = delete;
    //! No move
    syntax_base(syntax_base&&) = delete;
    //! Virtual desctructor, because this is a polymorphic base class
    virtual ~syntax_base() = 0;
    //! No copy
    syntax_base& operator=(const syntax_base&) = delete;
    //! No move
    syntax_base& operator=(syntax_base&&) = delete;
    //! Parses a script source text
    /*! After a successful return, the resulting internal representation can be
     * obtained from \a builder. If an exception is thrown, \a builder should
     * be treated as invalid and should be discarded. The \a file is not
     * accessed during parsing. It is expected that its content is provided in
     * \a src. The file name is only stored in \a builder for later reporting
     * of code locations, or it is recorded in a thrown exception.
     * \param[in,out] builder a builder object used to create the internal
     * representation of the parsed script
     * \param[in] src the source code to be parsed
     * \param[in] file an optional file name, which will be stored in the
     * internal representation of the script
     * \throw exception::parse_error if parsing fails */
    void parse(script_builder& builder, std::string_view src,
               std::string_view file = {});
    //! Parses a script file
    /*! It reads the \a file and passes it to parse().
     * \param[in,out] builder a builder object used to create the internal
     * representation of the parsed script
     * \param[in] file the file name
     * \throw exception::parse_error if parsing fails
     * \throw std::ios_base::failure if reading of \a file fails */
    void parse_file(script_builder& builder, std::string_view file);
protected:
    //! Invokes the parser.
    /*! It must be overriden in each class derived from syntax_base and it must
     * run a parser for the selected syntax.
     * \param[in] builder passed by parse()
     * \param[in] src passed by parse()
     * \throw exception::parse_error if parsing fails */
    virtual void run_parser(script_builder& builder, std::string_view src) = 0;
};

//! The interface for creating a running a selected parser
/*! This class allows to choose a parser class derived from syntax_base,
 * and to create its instance. */
class syntax_factory {
public:
    //! The name of parser syntax::canon
    static constexpr std::string_view syntax_canon{"canon"};
    //! Creates a parser by name
    /*! \param[in] syntax the parser name
     * \return a new instance of the parser; \c nullptr if \a syntax is not
     * registered in _registry */
    static std::unique_ptr<syntax_base> create(std::string_view syntax);
    //! Gets all known syntax names
    /*! \return a sorted view containing names registered in _registry */
    static std::ranges::view auto names() {
        return _registry | std::views::transform([](auto&& r){return r.first;});
    }
private:
    //! A function to create a parser object
    using make_fun = std::unique_ptr<syntax_base>(*)();
    //! The map from syntax names to parser classes
    static std::map<std::string_view, make_fun> _registry;
};

} // namespace threadscript
