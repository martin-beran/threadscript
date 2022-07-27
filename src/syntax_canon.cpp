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
#define RULE(name, body) decltype(body) name = body

    RULE(comment, rf::t('#') >> *rf::print() >> (rf::eof() | rf::nl()));
    RULE(space, (rf::lws() | comment)["space"sv]);

    // Cannot use a lambda, because of invalid macro expansion of the form
    // decltype(rule(lambda)) name = rule(lambda);
    static bool is_lit_char(char c) {
        return c >= 32 && c <= 126 && c != '"' && c != '\\';
    }
    RULE(lit_char, rf::p(is_lit_char));
    RULE(esc_name,
         rf::t('0') | rf::t('t') | rf::t('n') | rf::t('r') |
         rf::t('"') | rf::t('\\'));
    RULE(esc_hex, rf::str_ic("x"sv) >> rf::hex() >> rf::hex());
    RULE(esc_char, rf::t('\\') >> (esc_name | esc_hex));
    RULE(string_char, lit_char | esc_char);

    RULE(node_null, rf::str("null"sv));
    RULE(node_bool, rf::str("false"sv) | rf::str("true"sv));
    RULE(node_unsigned, rf::uint());
    RULE(node_int, (rf::t('+') | rf::t('-')) >> rf::uint());
    RULE(node_string, rf::t('"') >> *string_char >> rf::t('"'));
    RULE(node_val,
         node_null | node_bool | node_unsigned | node_int | node_string);

    RULE(node, rf::dyn()["node"sv]);

    RULE(params,
         *space >> -(node >> *space >> *(rf::t(',') >> *space >> node)));
    RULE(node_fun, rf::id() >> *space >> rf::t('(') >> *params >> rf::t(')'));
    
    RULE(_node, node_val | node_fun);

    RULE(script, (*space >> node >> *space)["script"sv]);

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
