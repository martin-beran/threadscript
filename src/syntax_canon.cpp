/*! \file
 * \brief The implementation part of threadscript/syntax_canon.hpp
 */

#include "threadscript/syntax_canon.hpp"

namespace threadscript::syntax {

/*** canon::rules ************************************************************/

using namespace std::string_view_literals;

//! Internal structure holding parser rules
struct canon::rules {
    //! The script builder to be used by handlers
    /*! Its value is valid only during canon::run_parser(). */
    script_builder* b = nullptr;
    //! [Grammar]
    //! \cond
#define RULE(name, body) decltype(body) name = (body)(std::string_view(#name))

    RULE(comment,
         rf::t('#') >> *rf::print() >> (rf::eof() | rf::nl()));
    RULE(space,
         (rf::lws() | comment)["Whitespace or comment expected"sv]);

    // Cannot use a lambda, because of invalid macro expansion of the form
    // decltype(rule(lambda)) name = rule(lambda);
    static bool is_lit_char(char c) {
        return c >= 32 && c <= 126 && c != '"' && c != '\\';
    }
    RULE(lit_char,
         rf::p(is_lit_char));
    RULE(esc_name,
         rf::t('0') | rf::t('t') | rf::t('n') | rf::t('r') |
         rf::t('"') | rf::t('\\'));
    RULE(esc_hex,
         rf::str_ic("x"sv) >> rf::hex() >> rf::hex());
    RULE(esc_char,
         rf::t('\\')("backslash"sv) >> (esc_name | esc_hex)
         ["Invalid escape"sv]);
    RULE(string_char,
         lit_char | esc_char);

    RULE(val_null,
         rf::str("null"sv));
    RULE(val_bool,
         rf::str("false"sv) | rf::str("true"sv));
    RULE(val_unsigned,
         rf::uint());
    RULE(val_int,
         (rf::t('+') | rf::t('-'))("Sign"sv) >> rf::uint()
         ["Expected number"sv]);
    RULE(val_string,
         rf::t('"') >> *string_char >> rf::t('"')["Expected '\"'"sv]);
    RULE(node_val,
         val_null | val_bool | val_unsigned | val_int | val_string);

    RULE(node,
         rf::dyn()
         ["Expected value or function"sv]);

    RULE(params,
         *space >> -(node >> *space >> *(rf::t(',') >> *space >> node)));
    RULE(node_fun,
         rf::id() >> *space >>
          rf::t('(')["Expected '('"sv] >>
          params >> rf::t(')')["Expected ',' or ')'"]);

    RULE(_node,
         node_val | node_fun);

    RULE(script,
         *space >> node >> *space);

    //! \endcond
    //! Configures rules
    rules() {
        node >>= _node;
    }
    //! [Grammar]
};

/*** canon *******************************************************************/

canon::canon(): _rules(std::make_unique<rules>())
{
}

canon::~canon() = default;

void canon::run_parser(script_builder& builder, std::string_view src,
                       parser::context::trace_t trace)
{
    _rules->b = &builder;
    parser::context ctx;
    ctx.trace = std::move(trace);
    ctx.parse(_rules->script, parser::make_script_iterator(src));
}

} // namespace threadscript::syntax
