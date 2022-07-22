#pragma once

/*! \file
 * \brief Common interface to parsers for syntax variants
 *
 * This file contains a base class for parsers of ThreadScript syntax variants.
 * It also provides an interface for parser selection and invoking the selected
 * parser on a source file.
 */

namespace threadscript {

//! The base class of parsers declared in namespace threadscript::syntax
/*! It is an abstract class. */
class syntax_base {
public:
    //! The default constructor
    syntax_base() = default;
    //! Virtual desctructor, because this is a polymorphic base class
    virtual ~syntax_base() = 0;
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
protected:
    //! Invokes the parser.
    /*! It must be overriden in each class derived from syntax_base and it must
     * run a parser for the selected syntax.
     * \param[in] builder passed by parse()
     * \param[in] file passed by parse()
     * \throw exception::parse_error if parsing fails */
    virtual void run_parser(script_builder& builder, std::string_view file) = 0;
};

//! The interface for creating a running a selected parser
/*! This class allows to choose a parser class derived from syntax_base,
 * and to create its instance. */
class syntax_factory {
public:
    //! Creates a parser by name
    /*!
     * \param[in] syntax the parser name
     * \return a new instance of the parser; \c nullptr if \a syntax is not
     * registered in _registry */
    static std::unique_ptr<syntax_base> create(std::string_view syntax);
private:
    //! The map from syntax names to parser classes
    static std::map<std::string_view, std::unique_ptr<syntax_base>(*)()>
        _registry;
};

} // namespace threadscript
