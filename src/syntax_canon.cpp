/*! \file
 * \brief The implementation part of threadscript/syntax_canon.hpp
 */

#include "threadscript/syntax_canon.hpp"

namespace threadscript::syntax {

/*** canon::rules ************************************************************/

//! Internal structure holding parser rules
struct canon::rules {
    //! Creates all rules and attaches them to \ref script
    rules();
    //! The script builder to be used by handlers
    /*! Its value is valid only during canon::run_parser(). */
    script_builder* b = nullptr;
    //! The top-level rule (a complete script)
    decltype(rf::dyn()) script;
};

canon::rules::rules()
{
    //! [Grammar]
    auto _script = rf::fail();
    script >>= _script;
    //! [Grammar]
}

/*** canon *******************************************************************/

canon::canon(): _rules(std::make_unique<rules>())
{
}

canon::~canon() = default;

void canon::run_parser(script_builder& builder, std::string_view src)
{
    _rules->b = &builder;
    parser::context ctx;
    ctx.parse(_rules->script, parser::make_script_iterator(src));
}

} // namespace threadscript::syntax
