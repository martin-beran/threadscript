/*! \file
 * \brief The implementation part of threadscript/syntax_canon.hpp
 */

#include "threadscript/syntax_canon.hpp"
#include "threadscript/code_builder.hpp"

namespace threadscript::syntax {

/*** canon::rules ************************************************************/

using namespace std::string_view_literals;

//! Defines a single rule as a member variable.
/*! It is needed to eliminate repeating of rule body definition in the rule
 * type, because non-static member variables cannot have type \c auto. It also
 * sets the rule name for tracing.
 * \param[in] name the rule name
 * \param[in] body the rule body
 * \note This macro prevents using lambdas as parameters of rules, because the
 * resulting macro expansion:
 * \code
 * decltype(rule(lambda)) name = rule(lambda);
 * \endcode
 * would be invalid. Although their definitions are identical, the two lambdas
 * in \c decltype and in the initializer are two different closure classes.
 * Hence, a named function, e.g., \c is_lit_char() must be used instead of a
 * lambda. */
// NOLINTNEXTLINE: This macro is needed as is to generate rules
#define RULE(name, body) decltype(body) name = (body)(std::string_view(#name))

//! Internal structure holding parser rules
/*! The grammar is defined by rules implemented as data members of this class,
 * declared using helper macro #RULE. The complete grammar is displayed and
 * documented in \ref Canonical_syntax. */
struct canon::rules {
    //! The temporary context used by rules
    struct tmp_ctx {
        //! Used to construct the top level context
        /*! \param[in] builder the script builder object */
        tmp_ctx(script_builder& builder): builder(builder) {}
        //! Creates a child context.
        /* It passes \ref builder and \ref node down from the parent to the
         * child context.
         * \param[in] up a parent context; must not be \c nullptr */
        tmp_ctx(tmp_ctx* up):
            builder((assert(up), up->builder)), node(up->node) {}
        //! The script builder to be used by handlers
        /*! Its value is valid only during canon::run_parser(). */
        script_builder& builder;
        //! The current node being built
        script_builder::node_handle node;
    };
    //! The rule factory used by the parser
    using rf =
        parser_ascii::rules::factory<iterator_type, parser::context, tmp_ctx>;
    //! The handler for rule \c val_null
    /*! \param[in] ctx the parsing context
     * \param[in] self the temporary context of this rule
     * \param[in] up the temporary context of the parent rule; it is never \c
     * nullptr
     * \param[in] info information about the parsed part of input
     * \param[in] begin the beginning of the matching input
     * \param[in] begin the end of the matching input */
    static void hnd_null(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                         parser::empty info,
                         iterator_type begin, iterator_type end);
    //! [Grammar]
    //! \cond
    RULE(comment,
         rf::t('#') >> *rf::print() >> (rf::eof() | rf::nl()));
    RULE(space,
         (rf::lws() | comment)["Whitespace or comment expected"sv]);

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
         rf::t('"') >> *string_char >> rf::t('"')
         ["Expected '\"'"sv]);
    RULE(node_val,
         val_null | val_bool | val_unsigned | val_int | val_string);

    RULE(node,
         rf::dyn()
         ["Expected value or function"sv]);

    RULE(params,
         rf::dyn());
    RULE(_params,
         node >> *space >> (rf::t(')')("close"sv) |
                            rf::t(',')["Expected ',' or ')'"sv]("comma"sv) >>
                            *space >> params));

    RULE(node_fun,
         rf::id() >> *space >>
          rf::t('(')["Expected '('"sv]("open"sv) >> *space >>
          (rf::t(')')("close"sv) | params));

    RULE(_node,
         node_val | node_fun);

    RULE(script,
         *space >> node >> *space >> rf::eof()
         ["Whitespace or comment expected"sv]);

    //! \endcond
    //! Configures rules
    rules() {
        params >>= _params;
        node >>= _node;
        val_null[hnd_null];
    }
    //! [Grammar]
};

void canon::rules::hnd_null(parser::context&, tmp_ctx& self, tmp_ctx*,
                            parser::empty, iterator_type begin, iterator_type)
{
    self.builder.add_node(self.node, file_location(begin.line, begin.column),
                          ""sv, self.builder.create_value_null());
}

/*** canon *******************************************************************/

canon::canon(): _rules(std::make_unique<rules>())
{
}

canon::~canon() = default;

void canon::run_parser(script_builder& builder, std::string_view src,
                       parser::context::trace_t trace)
{
    rules::tmp_ctx root_ctx(builder);
    parser::context ctx;
    ctx.trace = std::move(trace);
    ctx.parse(_rules->script, &root_ctx, parser::make_script_iterator(src));
}

} // namespace threadscript::syntax
