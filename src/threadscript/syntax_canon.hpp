#pragma once

/*! \file
 * \brief The parser for ThreadScript canonical syntax
 *
 * \test in file test_syntax_canon.cpp
 */

#include "threadscript/syntax.hpp"

namespace threadscript::syntax {

//! The parser for ThreadScript canonical syntax
/*! \test in file test_syntax_canon.cpp */
class canon final: public syntax_base {
public:
    //! Creates implementation of rules.
    canon();
    //! Destroys the implementation of rules.
    /*! \note It is explicitly defaulted in syntax_canon.cpp, because it needs
     * complete type \ref rules in order to be able to generate the destructor
     * of \c std::unique_ptr<rules>. */
    ~canon() override;
protected:
    void run_parser(script_builder& builder, std::string_view src) override;
private:
    struct rules;
    std::unique_ptr<rules> _rules; //!< PImpl (rules)
};

} // namespace threadscript::syntax
