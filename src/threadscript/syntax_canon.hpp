#pragma once

/*! \file
 * \brief The parser for ThreadScript canonical syntax
 *
 * \test in file test_syntax_canon.cpp
 */

#include "threadscript/syntax.hpp"

//! The namespace containing parsers for variants of ThreadScript syntax
namespace threadscript::syntax {

//! The parser for ThreadScript canonical syntax
/*! \test in file test_syntax_canon.cpp */
class canon final: public syntax_base {
protected:
    void run_parser(script_builder& builder, std::string_view file) override;
};

} // namespace threadscript::syntax
