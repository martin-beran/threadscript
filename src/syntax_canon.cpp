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
 * documented in \ref Canonical_syntax.
 * \todo Binary and hexadecimal integer literals */
struct canon::rules {
    //! The temporary context used by rules
    struct tmp_ctx {
        //! Used to construct the top level context
        /*! \param[in] builder the script builder object */
        explicit tmp_ctx(script_builder& builder): builder(builder) {}
        //! Creates a child context.
        /*! It passes \ref builder and \ref node down from the parent to the
         * child context.
         * \param[in] up a parent context; must not be \c nullptr */
        explicit tmp_ctx(tmp_ctx* up):
            builder((assert(up), up->builder)), node(up->node), str(up->str) {}
        //! The script builder to be used by handlers
        /*! Its value is valid only during canon::run_parser(). */
        script_builder& builder;
        //! The current node being built
        script_builder::node_handle node;
        //! The current string being built
        std::shared_ptr<std::string> str;
    };
    //! The rule factory used by the parser
    using rf =
        parser_ascii::rules::factory<iterator_type, parser::context, tmp_ctx>;
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
    RULE(string_begin,
         rf::t('"'));
    RULE(string_data,
         *string_char);

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
         string_begin >> (string_data >> rf::t('"')["Expected '\"'"sv]));
    RULE(node_val,
         val_null | val_bool | val_unsigned | val_int | val_string);

    RULE(node,
         rf::dyn()
         ["Expected value or function"sv]);

    RULE(fun_name,
         rf::id());
    RULE(params,
         rf::dyn());
    RULE(_params,
         node >> *space >> (rf::t(')')("close"sv) |
                            rf::t(',')["Expected ',' or ')'"sv]("comma"sv) >>
                            *space >> params));
    RULE(fun_params,
         *space >> rf::t('(')["Expected '('"sv]("open"sv) >> *space >>
         (rf::t(')')("close"sv) | params));

    RULE(node_fun,
         fun_name >> fun_params);

    RULE(_node,
         node_val | node_fun);

    RULE(script,
         *space >> node >> *space >> rf::eof()
         ["Whitespace or comment expected"sv]);

    //! \endcond
    //! The handler for rule \c val_null
    /*! \param[in] ctx the parsing context
     * \param[in] self the temporary context of this rule
     * \param[in] up the temporary context of the parent rule; it is never \c
     * nullptr
     * \param[in] info information about the parsed part of input
     * \param[in] begin the beginning of the matching input
     * \param[in] end the end of the matching input */
    static void hnd_null(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                         decltype(val_null)::info_type info,
                         iterator_type begin, iterator_type end);
    //! The handler for rule \c val_bool
    /*! \copydetails hnd_null() */
    static void hnd_bool(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                         decltype(val_bool)::info_type info,
                         iterator_type begin, iterator_type end);
    //! The handler for rule \c val_int
    /*! \copydetails hnd_null() */
    static void hnd_int(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                        decltype(val_int)::info_type info,
                        iterator_type begin, iterator_type end);
    //! The handler for rule \c val_unsigned
    /*! \copydetails hnd_null() */
    static void hnd_unsigned(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                        decltype(val_unsigned)::info_type info,
                        iterator_type begin, iterator_type end);
    //! The handler for rule \c string_begin (used by \c val_string)
    /*! \copydetails hnd_null() */
    static void hnd_string_begin(parser::context& ctx, tmp_ctx& self,
                                 tmp_ctx* up,
                                 decltype(string_begin)::info_type info,
                                 iterator_type begin, iterator_type end);
    //! The handler for rule \c lit_char (used by \c val_string)
    /*! \copydetails hnd_null() */
    static void hnd_lit_char(parser::context& ctx, tmp_ctx& self,
                             tmp_ctx* up,
                             decltype(lit_char)::info_type info,
                             iterator_type begin, iterator_type end);
    //! The handler for rule \c lit_esc_name (used by \c val_string)
    /*! \copydetails hnd_null() */
    static void hnd_esc_name(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                             decltype(esc_name)::info_type info,
                             iterator_type begin, iterator_type end);
    //! The handler for rule \c lit_esc_hex (used by \c val_string)
    /*! \copydetails hnd_null() */
    static void hnd_esc_hex(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                            decltype(esc_hex)::info_type info,
                            iterator_type begin, iterator_type end);
    //! The handler for rule \c string_data (used by \c val_string)
    /*! \copydetails hnd_null() */
    static void hnd_string(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                           decltype(string_data)::info_type info,
                           iterator_type begin, iterator_type end);
    //! The handler for rule \c fun_name (used by \c node_fun)
    /*! \copydetails hnd_null() */
    static void hnd_fun(parser::context& ctx, tmp_ctx& self, tmp_ctx* up,
                        decltype(fun_name)::info_type info,
                        iterator_type begin, iterator_type end);
    //! Configures rules
    rules() {
        params >>= _params;
        node >>= _node;
        val_null[hnd_null];
        val_bool[hnd_bool];
        val_int[hnd_int];
        val_unsigned[hnd_unsigned];
        string_begin[hnd_string_begin];
        lit_char[hnd_lit_char];
        esc_name[hnd_esc_name];
        esc_hex[hnd_esc_hex];
        string_data[hnd_string];
        fun_name[hnd_fun];
    }
    //! [Grammar]
};

void canon::rules::hnd_bool(parser::context&, tmp_ctx& self, tmp_ctx*,
                            decltype(val_bool)::info_type,
                            iterator_type begin, iterator_type end)
{
    assert(begin != end);
    self.builder.add_node(self.node, file_location(begin.line, begin.column),
                          ""sv, self.builder.create_value_bool(*begin == 't'));
}

void canon::rules::hnd_esc_hex(parser::context&, tmp_ctx& self, tmp_ctx*,
                               decltype(esc_hex)::info_type,
                               iterator_type begin, iterator_type end)
{
    assert(self.str);
    assert(std::distance(begin, end) == 3);
    self.str->push_back(16 * parser_ascii::hex_to_int(*std::next(begin, 1)) +
                        parser_ascii::hex_to_int(*std::next(begin, 2)));
}

void canon::rules::hnd_esc_name(parser::context&, tmp_ctx& self, tmp_ctx*,
                                decltype(esc_name)::info_type,
                                iterator_type begin, iterator_type end)
{
    assert(self.str);
    assert(std::distance(begin,end) == 1);
    switch (*begin) {
    case '0':
        self.str->push_back('\0');
        break;
    case 't':
        self.str->push_back('\t');
        break;
    case 'n':
        self.str->push_back('\n');
        break;
    case 'r':
        self.str->push_back('\r');
        break;
    case '"':
        self.str->push_back('"');
        break;
    case '\\':
        self.str->push_back('\\');
        break;
    default:
        assert(false);
    }
}

void canon::rules::hnd_fun(parser::context&, tmp_ctx&, tmp_ctx* up,
                           decltype(fun_name)::info_type,
                           iterator_type begin, iterator_type end)
{
    // Logic of this handler depends on fun_name to be a direct child of
    // node_fun. This is the reason why node_fun is defined using
    // fun_name >> fun_params. If the bodies of fun_name and fun_params were
    // directly concatenated in the body of node_fun, we would have to go up
    // several levels from fun_name to node_fun in order to call add_node() at
    // the correct point of the parse tree.
    up->node =
        up->builder.add_node(up->node, file_location(begin.line, begin.column),
                             std::string_view(&*begin,
                                              std::distance(begin, end)));
}

void canon::rules::hnd_int(parser::context&, tmp_ctx& self, tmp_ctx*,
                           decltype(val_int)::info_type,
                           iterator_type begin, iterator_type end)
{
    using limits = std::numeric_limits<config::value_int_type>;
    try {
        auto val = std::stoll(std::string(begin, end));
        if (val >= limits::min() && val <= limits::max()) {
            self.builder.add_node(self.node,
                                  file_location(begin.line, begin.column),
                                  ""sv, self.builder.create_value_int(val));
            return;
        }
    } catch (std::invalid_argument&) {
    } catch (std::out_of_range&) {
    }
    throw parser::error(begin, "Invalid number");
}

void canon::rules::hnd_lit_char(parser::context&, tmp_ctx& self, tmp_ctx*,
                                decltype(lit_char)::info_type,
                                iterator_type begin, iterator_type end)
{
    assert(self.str);
    assert(begin != end);
    self.str->push_back(*begin);
}

void canon::rules::hnd_null(parser::context&, tmp_ctx& self, tmp_ctx*,
                            decltype(val_null)::info_type,
                            iterator_type begin, iterator_type)
{
    self.builder.add_node(self.node, file_location(begin.line, begin.column),
                          ""sv, script_builder::create_value_null());
}

void canon::rules::hnd_string(parser::context&, tmp_ctx& self, tmp_ctx*,
                              decltype(string_data)::info_type,
                              iterator_type begin, iterator_type)
{
    assert(self.str);
    self.builder.add_node(self.node, file_location(begin.line, begin.column),
        ""sv, self.builder.create_value_string(*self.str));
}

void canon::rules::hnd_string_begin(parser::context&, tmp_ctx&, tmp_ctx* up,
                                    decltype(string_begin)::info_type,
                                    iterator_type, iterator_type)
{
    up->str = std::make_shared<std::string>();
}

void canon::rules::hnd_unsigned(parser::context&, tmp_ctx& self, tmp_ctx*,
                                decltype(val_unsigned)::info_type,
                                iterator_type begin, iterator_type end)
{
    using limits = std::numeric_limits<config::value_unsigned_type>;
    try {
        auto val = std::stoull(std::string(begin, end));
        if (val >= limits::min() && val <= limits::max()) {
            self.builder.add_node(self.node,
                                  file_location(begin.line, begin.column),
                                  ""sv,
                                  self.builder.create_value_unsigned(val));
            return;
        }
    } catch (std::invalid_argument&) {
    } catch (std::out_of_range&) {
    }
    throw parser::error<iterator_type>(begin, "Invalid number");
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
