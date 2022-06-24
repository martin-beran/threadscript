#pragma once

/*! \file
 * \brief A generic recursive descent parser
 *
 * \test in file test_parser.cpp
 */

#include <iterator>

//! The namespace containing a generic recursive descent parser
/*! This is not a ThreadScript parser, it is a framework for creating parsers.
 * \test in file test_parser.cpp */
namespace threadscript::parser {

//! An exception thrown if parsing fails
/*! It stores a position of the error in the input sequence and an error
 * message.
 * \tparam I an iterator used to specify a position in the input sequence */
template <std::forward_iterator I> class error: public std::runtime_error {
public:
    //! Creates the exception
    /*! \param[in] pos error position
     * \param[in] msg error message */
    error(I pos, const std::string& msg): runtime_error(msg), _pos(pos) {}
    //! Gets the error position
    /*! \return the position */
    I pos() const { return _pos; }
private:
    I _pos; //!< Stored error position
};

//! A grammar that defines a specific parser
/*! \tparam Term the type of terminal symbols of the grammar */
template <class Term = char> class grammar {
};

//! A parsing context is an instance of a parser
/*! This is the engine that parses an input sequence of terminal symbols
 * according to a \ref grammar.
 * \tparam Term the type of terminal symbols of the grammar */
template <class Term = char> class context {
public:
    //! The type of grammar used by this parser
    using grammar_type = grammar<Term>;
    //! Creates the parsing context
    /*! \param[in] rules the grammar rules that control this parser */
    explicit context(const grammar_type& rules): rules(rules) {}
    //! Runs the parser
    /*! It parses the input sequence of terminals between \a begin and \a end.
     * \param[in] begin the start of the parsed sequence
     * \param[in] end the end of the parsed sequence
     * \throw \ref error if parsing fails */
    template <std::forward_iterator I> void parse(I begin, I end);
private:
    const grammar_type& rules; //!< The grammar that controls this parser
};

} // namespace threadscript::parser
