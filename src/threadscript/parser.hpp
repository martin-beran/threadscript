#pragma once

/*! \file
 * \brief A generic recursive descent parser
 *
 * \test in file test_parser.cpp
 */

#include "threadscript/finally.hpp"
#include "threadscript/debug.hpp"

#include <cassert>
#include <concepts>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>

//! The namespace containing a generic recursive descent parser
/*! This is not a ThreadScript parser, it is a framework for creating parsers.
 * \test in file test_parser.cpp */
namespace threadscript::parser {

class context;

//! An exception thrown if parsing fails
/*! It stores a position of the error in the input sequence and an error
 * message.
 * \tparam I an iterator used to specify a position in the input sequence */
template <std::forward_iterator I> class error: public std::runtime_error {
public:
    //! Creates the exception
    /*! \param[in] pos error position
     * \param[in] msg error message */
    explicit error(I pos, std::string_view msg = "Parse error"):
        runtime_error(std::string{msg}), _pos(pos) {}
    //! Gets the error position
    /*! \return the position */
    I pos() const { return _pos; }
private:
    I _pos; //!< Stored error position
};

//! A result of a parsing rule evaluation
enum class rule_result: uint8_t {
    //! The rule does not match, but an alternative can match
    fail,
    //! The rule matches, but an alternative can be tried later
    ok,
    //! The rule does not match and an alternative cannot be tried
    /*! It disables the following alternatives. This is normally used by a
     * sequence node (rules::seq)
     * to signal to its parent disjunction node (rules::alt) that parent's
     * remaining alternatives should not be evaluated. */
    fail_final_seq,
    //! The rule does not match and an alternative cannot be tried
    /*! It disables the following alternatives. This is normally used by a
     * disjunction node (rules::alt) to signal to its parent disjunction node
     * that parent's remaining alternatives should not be evaluated. */
    fail_final_alt,
    //! The matches, and an alternative will not be tried.
    /*! It disables the following alternatives. This is normally used by a node
     * to signal to its parent sequence node that if a later node in the
     * sequence fails, the sequence should return fail_final_seq instead of
     * rule_result::fail to its parent disjunction node. */
    ok_final,
};

//! A predicate to test a terminal symbol
/*! A predicate gets a terminal symbol from the input sequence and checks a
 * condition for it.
 * \tparam Predicate a predicate
 * \tparam It an iterator to the input sequence of terminal symbols */
template <class Predicate, class It>
concept predicate =
    std::is_invocable_r_v<bool, Predicate,
        typename std::iterator_traits<It>::value_type> &&
    std::forward_iterator<It>;

//! A predicate to compare two terminal symbols
/*! A predicate gets two terminal symbols, usually from two sequences, and
 * checks a condition for it.
 * \tparam Predicate a predicate
 * \tparam It an iterator type */
template <class Predicate, class It>
concept predicate2 =
    std::is_invocable_r_v<bool, Predicate,
        typename std::iterator_traits<It>::value_type,
        typename std::iterator_traits<It>::value_type> &&
    std::forward_iterator<It>;

//! A handler called when a rule matches
/*! A handler gets a context object \a Ctx that has invoked this rule via
 * context::parse(), some information \a Info extracted from the input by the
 * rule (e.g., a numeric value obtained by parsing a sequence of digits), and
 * iterators \a It delimiting the part of the input matched by the rule.
 * \tparam Handler a handler
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of a rule
 * \tparam Up a temporary context of a parent rule (will be \c nullptr if a
 * rule has no parent)
 * \tparam Info information about a parsed part of input; it is expected to be
 * \ref threadscript::parser::empty unless specified otherwise for a particular
 * rule type
 * \tparam It an iterator to the input sequence of terminal symbols */
template <class Handler, class Ctx, class Self, class Up, class Info, class It>
concept handler =
    std::is_invocable_r_v<void, Handler, Ctx&, Self&, Up*, Info, It, It> &&
    std::forward_iterator<It> && std::derived_from<Ctx, context>;

//! The default handler called when a rule matches
/*! \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam Info information about a parsed part of input
 * \tparam It an iterator to the input sequence of terminal symbols
 * \param[in] ctx a parsing context
 * \param[in] self a temporary context of this rule
 * \param[in] up a temporary context of a parent rule; \c nullptr if this rule
 * hans no parent rule
 * \param[in] info information about a parsed part of input
 * \param[in] begin the first terminal matching a rule
 * \param[in] end one after the last terminal matching a rule */
template <class Ctx, class Self, class Up, class Info, std::forward_iterator It>
using default_handler = std::function<void(Ctx& ctx, Self& self, Up* up,
                                           Info info, It begin, It end)>;

//! Changes the type of information passed to a handler
/*! For supported \a Handler types, it contains a member typedef \c type, which
 * is the modifier \a Handler.
 * \tparam Handler a handler type
 * \tparam Info2 a new type of parameter \a info of \a Handler */
template <class Handler, class Info2>
struct rebind_handler {};

//! A modified \a Handler type with \a info parameter type changed to \a Info2
/*! \tparam Handler a handler type
 * \tparam Infor2 a new type of parameter \a info */
template <class Handler, class Info2>
using rebind_handler_t =
    typename rebind_handler<Handler, Info2>::type;

//! A template specialization for rebinding default_handler
/*! \tparam Ctx a context type
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam Info a the original information type
 * \tparam It an iterator type
 * \tparam Info2 a new information type */
template <class Ctx, class Self, class Up, class Info, class It, class Info2>
struct rebind_handler<default_handler<Ctx, Self, Up, Info, It>, Info2> {
    //! The hew handler type
    using type = default_handler<Ctx, Self, Up, Info2, It>;
};

//! Empty data that can be used as parameter \a Info of a handler.
/*! It is used by default as \a Info if no additional information is produced
 * by a rule, and also as internal temporary data \a Tmp of a rule if the rule
 * does not need any additional data. */
struct empty {};

//! Creates a typed null pointer.
/*! It is intended for creating argument \a Up for a deduction guide of a rule
 * class in namespace threadscript::rules
 * \tparam Up a temporary context of a parent rule
 * \return a null pointer to \a Up */
template <class Up> Up* up_null()
{
    return static_cast<Up*>(nullptr);
}

//! Get a parent temporary context from a child temporary context
/*! This primary template returns the same type as is used by the current rule,
 * with optional reference, \c const, or \c volatile removed.
 * \tparam T a temporary context type of a rule */
template <class T> struct up_type {
    //! The temporary context type of a parent rule
    using type = std::remove_cvref_t<T>;
};

//! A template specialization for a context type providing a parent type
/*! This specialization uses \c std::remove_cvref_t<T>::up_type to get the
 * parent context type */
template <class T>
    requires requires { typename std::remove_cvref_t<T>::up_type; }
struct up_type<T> {
    //! The temporary context type of a parent rule
    using type = typename std::remove_cvref_t<T>::up_type;
};

//! Get a parent temporary context from a child temporary context
/*! If \a T, with optional reference, \c const, or \c volatile removed, has
 * member type \c up_type, then it is returned. Otherwise, \a T is returned.
 * \tparam T a temporary context type of a rule */
template <class T> using up_type_t = typename up_type<T>::type;

//! A base class of all rules
/*! Member functions in this class are not virtual, but they can be overriden
 * in a derived class \a Rule, because they are called via \c this pointer
 * static-cast to a pointer to \a Rule.
 *
 * Unless the output type \a Info or the type of temporary private data created
 * by parse_internal() is changed, both these types are \ref empty.
 * \tparam Rule the derived rule class
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam Info information about a parsed part of input
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches
 * \threadsafe{safe,unsafe} */
template <class Rule, class Ctx, class Self, class Up, class Info,
    std::forward_iterator It,
    handler<Ctx, Self, Up, Info, It> Handler =
        default_handler<Ctx, Self, Up, Info, It>>
class rule_base {
public:
    using rule_type = Rule; //!< The derived rule type
    //! TThe rule_type without reference, \c const, and \c volatile
    using value_type = std::remove_cvref_t<Rule>;
    using context_type = Ctx; //!< A parsing context
    using self_ctx_type = Self; //!< A temporary context of this rule
    using up_ctx_type = Up; //!< A temporary context of a parent rule
    using info_type = Info; //!< Information about a parsed part of input
    using iterator_type = It; //!< An iterator to the input
    //! The type of a terminal symbol
    using term_type = typename std::iterator_traits<It>::value_type;
    //! The type of a handler called when the rule matches
    using handler_type = Handler;
    //! The result type of parse() and eval()
    using parse_result = std::pair<rule_result, It>;
    //! Creates the rule with a handler
    /*! \param[in] hnd the handler stored in the rule */
    explicit rule_base(Handler hnd = {}): hnd(std::move(hnd)) {}
    //! Parses a part of the input according to the rule and uses the result.
    /*! This function consists of several parts, mainly delegated to other
     * member functions:
     * \arg It creates a temporary context of type Self.
     * \arg parse_internal() creates temporary private data used during the
     * single call of parse()
     * \arg parse_with_tmp() parses the input by calling eval() and if a match
     * is found, it is further processed by attr()
     * \arg make_info() extracts information about the parsing result from
     * temporary private data into a handler argument
     * \arg attr() calls the registered handler and passes to it a result of
     * parsing; if no handler is registered, \a up is not \c nullptr, and \a
     * self has member function \c copy_up taking a \a Up& as a parameter,
     * attr() calls it
     * \arg parse_internal() destroys temporary private data and a temporary
     * context
     * \arg if the rule does not match, then attr(), make_info(), and the
     * handler are not called
     *
     * If the rule matches, it returns rule_result::ok or rule_result::ok_final
     * and the iterator denoting the end of the matched part of input. If the
     * rule does not match, it returns rule_result::fail,
     * rule_result::fail_final_seq, or rule_result::fail_final_alt and the
     * position where matching failed.
     *
     * If matching fails and error_msg is nonempty, it is stored in \a ctx and
     * the position of the error is stored in \a e_it. A descendant node may
     * overwrite the error message and position. This logic ensures reporting
     * an error where it occurred at the lowest level in the parse tree, not in
     * some ancestor node where the error propagates.
     *
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in,out] up a temporary context of a parent rule; \c nullptr if
     * this rule has no parent
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \param[in,out] e_it an iterator used to track an error location
     * \return the result of matching this rule with the input sequence
     * \throw error if the maximum depth of rule nesting is exceeded */
    parse_result parse(Ctx& ctx, Up* up, It begin, It end,
                       std::optional<It>& e_it) const;
    //! Sets a new handler
    /*! \param[in] h the new handler */
    void set_handler(Handler h) {
        hnd = std::move(h);
    }
    /*! \copydoc set_handler()
     * \return \c *this */
    Rule& operator[](Handler h) & {
        set_handler(std::move(h));
        return static_cast<Rule&>(*this);
    }
    /*! \copydoc set_handler()
     * \return \c *this */
    Rule operator[](Handler h) && {
        set_handler(std::move(h));
        return static_cast<Rule&&>(*this);
    }
    //! Sets error message.
    /*! \param[in] msg the new message */
    void set_error_msg(std::string_view msg) {
        error_msg = msg;
    }
    /*! \copydoc set_error_msg()
     * \return \c *this */
    Rule& operator[](std::string_view msg) & {
        set_error_msg(msg);
        return static_cast<Rule&>(*this);
    }
    /*! \copydoc set_error_msg()
     * \return \c *this */
    Rule operator[](std::string_view msg) && {
        set_error_msg(msg);
        return static_cast<Rule&&>(*this);
    }
    //! Sets rule name for tracing.
    /*! If \a name is nonempty and tracing is enabled in \ref context, this
     * rule will be traced. If \a name is empty, this rule will not be traced.
     * \param[in] name the rule name reported in tracing, empty to not trace
     * this rule. */
    void set_tracing(std::string_view name) {
        trace = name;
    }
    /*! \copydoc set_tracing()
     * \return \c *this */
    Rule& operator()(std::string_view name) & {
        set_tracing(name);
        return static_cast<Rule&>(*this);
    }
    /*! \copydoc set_tracing()
     * \return \c *this */
    Rule operator()(std::string_view name) && {
        set_tracing(name);
        return static_cast<Rule&&>(*this);
    }
    //! The stored handler called by attr().
    /*! The handler gets additional information created by make_info(). */
    Handler hnd{};
    //! The error message produced by this rule
    /*! If this rule fails and the err_msg is nonempty, it is set into
     * context::error_msg. */
    std::string error_msg{};
    //! The rule name used for tracing
    /*! The rule evaluation is reported when tracing is enabled in \ref
     * context. */
    std::string trace{};
protected:
    //! Creates temporary private data usable in this parsing operation.
    /*! Private data are destroyed when the current parsing operation ends.
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in,out] self the temporary context; may be modified by the
     * function
     * \param[in,out] up a temporary context of a parent rule; \c nullptr if
     * this rule has no parent
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \param[in,out] e_it an iterator used to track an error location
     * \return the result of matching this rule with the input sequence */
    parse_result parse_internal(Ctx& ctx, Self& self, Up* up, It begin, It end,
                                std::optional<It>& e_it) const
    {
        empty tmp{};
        return static_cast<const Rule*>(this)->parse_with_tmp(ctx, self, up,
                                                      tmp, begin, end, e_it);
    }
    //! Parses the input and can use temporary private data
    /*! When the rules matches, the result is passed out from the rule by
     * calling attr().
     * \tparam Tmp the type of temporary private data
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in,out] self the temporary context; may be modified by the
     * function
     * \param[in,out] up a temporary context of a parent rule; \c nullptr if
     * this rule has no parent
     * \param[in,out] tmp temporary private data
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \param[in,out] e_it an iterator used to track an error location
     * \return the result of matching this rule with the input sequence */
    template <class Tmp>
    parse_result parse_with_tmp(Ctx& ctx, Self& self, Up* up, Tmp& tmp,
                                It begin, It end, std::optional<It>& e_it) const
    {
        auto result = static_cast<const Rule*>(this)->eval(ctx, self, tmp,
                                                           begin, end, e_it);
        if (result.first == rule_result::ok ||
            result.first == rule_result::ok_final)
        {
            static_cast<const Rule*>(this)->attr(ctx, self, up, tmp, begin,
                                                 result.second);
        }
        return result;
    }
    //! Parses a part of the input according to the rule.
    /*! This function decides if the input matches the rule. The default
     * implementation does not consume any input and fails (with
     * rule_result::fail).
     * \tparam Tmp the type of temporary private data
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in,out] self the temporary context; may be modified by the
     * function
     * \param[in,out] tmp temporary private data
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \param[in,out] e_it an iterator used to track an error location
     * \return the result of matching this rule with the input sequence,
     * with the same meaning as in parse() */
    template <class Tmp>
    parse_result eval([[maybe_unused]] Ctx& ctx, [[maybe_unused]] Self& self,
                      [[maybe_unused]] Tmp& tmp,
                      [[maybe_unused]] It begin, [[maybe_unused]] It end,
                      [[maybe_unused]] std::optional<It>& e_it) const
    {
        return {rule_result::fail, begin};
    }
    //! Transforms temporary private data into the rule output
    /*! The default implementation simply passes \a tmp as an argument of
     * \a Info constructor.
     * \tparam Tmp the type of temporary private data
     * \param[in,out] tmp temporary private data
     * \return informatout about a parsed part of input */
    template <class Tmp> Info make_info(Tmp& tmp) const {
        return Info(tmp);
    }
    //! Handles the matching part of the input.
    /*! The default implementation calls the handler stored in the rule object.
     * If the handler is convertible to \c bool, as default_handler is, it is
     * called only if it evaluates to \c true.
     * \tparam Tmp the type of temporary private data
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in,out] self the temporary context; may be modified by the
     * function
     * \param[in,out] up a temporary context of a parent rule; \c nullptr if
     * this rule has no parent
     * \param[in,out] tmp temporary private data
     * \param[in] begin a position in the input sequence where matching starts;
     * it is the \a begin argument of parse()
     * \param[in] end the end of matching part of the input, as returned by
     * eval()
     * \note It is named \c attr, because its typical functionality is stored
     * a value obtained from the matching part of input into the attribute of
     * an attribute grammar. */
    template <class Tmp>
    void attr(Ctx& ctx, Self& self, Up* up, Tmp& tmp, It begin, It end) const {
        if constexpr (requires { bool(hnd); }) {
            if (bool(hnd))
                hnd(ctx, self, up,
                    static_cast<const Rule*>(this)->make_info(tmp), begin, end);
            else
                if constexpr (requires { self.copy_up(*up); })
                    if (up)
                        self.copy_up(*up);
        } else
            hnd(ctx, self, up, static_cast<const Rule*>(this)->make_info(tmp),
                begin, end);
    }
};

//! A rule, which must be derived from rule_base and have a \c parse() function
/*! \tparam Rule a rule type */
template <class Rule> concept rule = std::derived_from<Rule, rule_base<
    Rule, typename Rule::context_type, typename Rule::self_ctx_type,
    typename Rule::up_ctx_type, typename Rule::info_type,
    typename Rule::iterator_type, typename Rule::handler_type>> &&
    requires (const Rule r, typename Rule::context_type& ctx,
              typename Rule::up_ctx_type* up,
              const typename Rule::iterator_type it,
              std::optional<typename Rule::iterator_type>& e_it)
    {
        { r.parse(ctx, up, it, it, e_it) } ->
            std::same_as<typename Rule::parse_result>;
    };

//! A rule, which can also be a reference, \c const, or \c volatile
/*! \tparam Rule a rule type */
template <class Rule> concept rule_cvref = rule<std::remove_cvref_t<Rule>>;

//! Checks that \a RV is a \a Rule without a reference, \c const, \c volatile
/*! \tparam RV a rule type
 * \tparam Rule a rule type */
template <class RV, class Rule>
concept rule_value = std::same_as<RV, std::remove_cvref_t<Rule>>;

//! Like rebind_handler_t, but accepts a rule as the first argument
/*! \tparam Rule a rule type, possibly a reference, \c const, or \c volatile
 * \tparam Info2 a new type of parameter \a info of \a Handler */
template <rule_cvref Rule, class Info2> using rebind_rhnd_t =
    rebind_handler_t<typename std::remove_cvref_t<Rule>::handler_type, Info2>;

//! A parsing context
/*! A derived class may be used instead, providing additional functionality
 * available to rules and their handlers. A reference to this object is parsed
 * to rules during parsing. Rules may modify the context, e.g., set error_msg
 * to be reported if parsing fails.
 * \threadsafe{safe,unsafe} */
class context {
public:
    //! A function type used for rule tracing
    /*! If not empty, the target function is called when entering and exiting
     * each rule with nonempty rule_base::trace. Arguments \a begin_line, \a
     * begin_column, \a end_line, \a end_column are zero if access to the
     * parsed data uses an iterator which is not script_iterator. Otherwise,
     * they contain the position of
     * \arg begin and end of input data when entering a rule
     * \arg the matched part of input data if the rule matches
     * \arg the position of an error and the end of input data if the rule does
     * not match
     *
     * \param[in] result \c nullopt when entering a rule, a rule_result value
     * when returning from a rule
     * \param[in] name the value of rule_base::trace of the traced rule
     * \param[in] depth the stack depth of the rule
     * \param[in] error the error message of a failed node, empty if not failed
     * \param[in] begin_line reported beginning line number
     * \param[in] begin_column reported beginning column number
     * \param[in] end_line reported end line number
     * \param[in] end_column reported end column number
     * \param[in] err_line reported error line number
     * \param[in] err_column reported error column number */
    using trace_t = std::function<void(std::optional<rule_result> result,
                                       const std::string& name, size_t depth,
                                       std::string_view error,
                                       size_t begin_line, size_t begin_column,
                                       size_t end_line, size_t end_column,
                                       std::optional<size_t> err_line,
                                       std::optional<size_t> err_column)>;
    //! Parses an input sequence of terminal symbols according to \a rule.
    /*! \tparam R a rule type
     * \param[in] rule the rule matched to the input
     * \param[in,out] up passed to \a rule as a temporary context of a parent
     * rule; may be \c nullptr
     * \param[in] begin the start of the input
     * \param[in] end the end of the input
     * \param[in] all whether to require all input to match
     * \return the end of matching part of the input; it is equal to \a end if
     * \a all is \c true
     * \throw error if the input does not match the \a rule
     * \note Exceptions are used to signal failed parsing instead of a return
     * value, because an exception can be thrown directly from a deeper level
     * of nested rules, e.g., for exceeded maximum parsing depth. */
    template <rule R> typename R::iterator_type
    parse(const R& rule, typename R::up_ctx_type* up,
          typename R::iterator_type begin, typename R::iterator_type end,
          bool all = true)
    {
        depth = 0;
        std::optional<typename R::iterator_type> e_it{};
        switch (auto [result, pos] = rule.parse(*this, up, begin, end, e_it);
                result)
        {
            case rule_result::fail:
            case rule_result::fail_final_seq:
            case rule_result::fail_final_alt:
                if (error_msg)
                    throw error(e_it.value_or(pos), *error_msg);
                else
                    throw error(pos);
            case rule_result::ok:
            case rule_result::ok_final:
                if (all && pos != end)
                    throw error(pos, partial_msg);
                return pos;
            default:
                assert(false);
        }
    }
    //! Parses an input sequence of terminal symbols according to \a rule.
    /*! It passes \c nullptr to \a rule as a temporary context of a parent rule.
     * \tparam R a rule type
     * \param[in] rule the rule matched to the input
     * \param[in] begin the start of the input
     * \param[in] end the end of the input
     * \param[in] all whether to require all input to match
     * \return the end of matching part of the input; it is equal to \a end if
     * \a all is \c true
     * \throw error if the input does not match the \a rule
     * \note Exceptions are used to signal failed parsing instead of a return
     * value, because an exception can be thrown directly from a deeper level
     * of nested rules, e.g., for exceeded maximum parsing depth. */
    template <rule R> typename R::iterator_type
    parse(const R& rule, typename R::iterator_type begin,
          typename R::iterator_type end, bool all = true)
    {
        return parse(rule, nullptr, begin, end, all);
    }
    //! Parses an input sequence of terminal symbols according to \a rule.
    /*! \tparam R a rule type
     * \param[in] rule the rule matched to the input
     * \param[in,out] up passed to \a rule as a temporary context of a parent
     * rule; may be \c nullptr
     * \param[in] it contains begin and end of the input
     * \param[in] all whether to require all input to match
     * \return the end of matching part of the input; it is equal to \a
     * it.second if \a all is \c true
     * \throw error if the input does not match the \a rule */
    template <rule R> typename R::iterator_type
    parse(const R& rule, typename R::up_ctx_type* up,
          std::pair<typename R::iterator_type, typename R::iterator_type> it,
          bool all = true)
    {
        return parse(rule, up, it.first, it.second, all);
    }
    //! Parses an input sequence of terminal symbols according to \a rule.
    /*! It passes \c nullptr to \a rule as a temporary context of a parent rule.
     * \tparam R a rule type
     * \param[in] rule the rule matched to the input
     * \param[in] it contains begin and end of the input
     * \param[in] all whether to require all input to match
     * \return the end of matching part of the input; it is equal to \a
     * it.second if \a all is \c true
     * \throw error if the input does not match the \a rule */
    template <rule R> typename R::iterator_type
    parse(const R& rule,
          std::pair<typename R::iterator_type, typename R::iterator_type> it,
          bool all = true)
    {
        return parse(rule, nullptr, it.first, it.second, all);
    }
    //! An error message to be stored in an exception.
    /*! If not \c nullopt, then this message is used when a parsing failed
     * exception (\ref error) is thrown, unless a more specific message is
     * used. If \c nullopt, then a default message defined by \ref error is
     * used. This is normally manipulated by rules during parsing, therefore it
     * is non-owning in order to reduce copying of strings. */
    std::optional<std::string_view> error_msg{};
    //! If nont \c nullopt, then it defines the maximum stack depth of parsing.
    std::optional<size_t> max_depth{};
    //! An error message used if max_depth is exceeded
    std::string depth_msg{"Maximum parsing depth exceeded"};
    //! An error message used if only a part of the input has been matched.
    /*! It is used in an exception thrown if a rule matches only a part of the
     * input and argument \a all of parse() is \c true. */
    std::string partial_msg{"Partial match"};
    //! A function used for rule tracing
    trace_t trace;
    //! Generates a default tracing message
    /*! Each message is indented by \a depth spaces. It contains:
     * \arg an indicator of start or end of a rule evaluation
     * \arg the \a result value, if it is end of a rule evaluation
     * \arg the rule \a name
     * \arg the rule \a depth
     * \arg begin and end posititions when entering a rule, after match and end
     * positions if a rule succeeds, error and end positions if a rule fails
     * \arg a position of an error (if stored during parsing)
     * \arg an error message (if stored during parsing)
     *
     * It expects the same arguments as \ref trace
     * \param[in] result
     * \param[in] name
     * \param[in] depth
     * \param[in] error
     * \param[in] begin_line
     * \param[in] begin_column
     * \param[in] end_line
     * \param[in] end_column
     * \param[in] err_line reported error line number
     * \param[in] err_column reported error column number
     * \return a tracing message */
    static std::string trace_msg(std::optional<rule_result> result,
                                 const std::string& name, size_t depth,
                                 std::string_view error,
                                 size_t begin_line, size_t begin_column,
                                 size_t end_line, size_t end_column,
                                 std::optional<size_t> err_line,
                                 std::optional<size_t> err_column)
    {
        std::string msg(depth, ' ');
        if (result)
            switch (*result) {
            case rule_result::fail:
                msg += "<<<<<<<<<<<FAIL";
                break;
            case rule_result::ok:
                msg += "<<<<<<<<<<<<<OK";
                break;
            case rule_result::fail_final_seq:
                msg += "<FAIL_FINAL_SEQ";
                break;
            case rule_result::fail_final_alt:
                msg += "<FAIL_FINAL_ALT";
                break;
            case rule_result::ok_final:
                msg += "<<<<<<<OK_FINAL";
                break;
            default:
                break;
            }
        else
            msg += ">>>>>>>>>>>>>>>";
        msg.append(" ").append(name);
        msg.append(" [").append(std::to_string(depth)).append("]");
        if (begin_line != 0 || begin_column != 0 ||
            end_line != 0 || end_column != 0)
        {
            msg.append(" ").append(std::to_string(begin_line));
            msg.append(":").append(std::to_string(begin_column));
            msg.append("-").append(std::to_string(end_line));
            msg.append(":").append(std::to_string(end_column));
        }
        if (err_line || err_column) {
            msg.append(" ").append(std::to_string(err_line.value_or(0)));
            msg.append(":").append(std::to_string(err_column.value_or(0)));
        }
        if (!error.empty())
            msg.append(" ").append(error);
        return msg;
    }
private:
    size_t depth = 0; //!< The current stack depth of parsing
    //! Needed to manipulate context::depth
    template <class Rule, class Ctx, class Self, class Up, class Info,
        std::forward_iterator It, handler<Ctx, Self, Up, Info, It> Handler>
    friend class rule_base;
};

//! An iterator for parsing script source code.
/*! It can be used when using any text consisting of a sequence of \c char's.
 * It maintains the current line and column numbers, both starting at 1. It
 * expects lines delimited by <tt>'\\n'</tt>. There is helper function
 * make_script_iterator() for creating a pair of iterators.
 * \tparam It the underlying iterator type, which must point to a \c char */
template <std::forward_iterator It> requires
    requires (It it) { { *it } -> std::same_as<char&>; } ||
    requires (It it) { { *it } -> std::same_as<const char&>; }
class script_iterator: public std::forward_iterator_tag {
public:
    //! A required member of an iterator class
    using difference_type = std::iterator_traits<It>::difference_type;
    //! A required member of an iterator class
    using value_type = std::iterator_traits<It>::value_type;
    //! A required member of an iterator class
    using pointer = std::iterator_traits<It>::pointer;
    //! A required member of an iterator class
    using reference = std::iterator_traits<It>::reference;
    //! A required member of an iterator class
    using iterator_category = typename std::forward_iterator_tag;
    //! Creates a singular iterator.
    /*! Possible uses of a default constructed script_iterator are determined
     * by type \a It. */
    script_iterator() = default;
    //! Creates a script_iterator from an underlying iterator.
    /*! \param[in] it the underlying iterator.
     * \param[in] line the initial line number
     * \param[in] column the initial column number */
    explicit script_iterator(It it, size_t line = 1, size_t column = 1):
        line(line), column(column), it(std::move(it)) {}
    //! Gets the value of the underlying iterator.
    /*! \return the underlying iterator */
    It get() const { return it; }
    //! Dereferences the underlying iterator.
    /*! \return the character referenced by the underlying iterator. */
    auto& operator*() const { return *it; }
    //! Compares two iterators for equality
    /*! \param[in] o another iterator
     * \return whether the underlying iterator values are equal */
    bool operator==(const script_iterator& o) const {
        return it == o.it;
    }
    //! Prefix increment of the iterator.
    /*! It adjusts the line and column numbers.
     * \return \c *this */
    script_iterator& operator++() {
        step();
        ++it;
        return *this;
    }
    //! Postfix increment of the iterator.
    /*! It adjusts the line and column numbers.
     * \return the value before incrementing the position. */
    script_iterator operator++(int) {
        auto result = *this;
        step();
        ++it;
        return std::move(result);
    }
    size_t line = 1; //!< The current line number (starting at 1)
    size_t column = 1; //!< The current column number (starting at 1)
private:
    //! Adjusts the line and column numbers when incrementing the iterator.
    void step() {
        if (*it == '\n') {
            ++line;
            column = 1;
        } else
            ++column;
    }
    It it; //!< The stored underlying iterator value
};

//! Checks if \a T is an instance of script_iterator.
/*! \tparam T a type */
template <class T> struct is_script_iterator: std::false_type {};

//! Checks if \a T is an instance of script_iterator.
/*! \tparam It an iterator type */
template <class It> struct is_script_iterator<script_iterator<It>>:
    std::true_type {};

//! Checks if \a T is an instance of script_iterator.
/*! \tparam T a type */
template <class T> inline constexpr bool is_script_iterator_v =
    is_script_iterator<T>::value;

//! Creates a pair of iterators denoting contents of a text container.
/*! The result can be directly passed to context::parse().
 * \tparam T a text container, i.e., a class containing a sequence of
 * characters
 * \param[in] chars the text container
 * \return begin and end of the sequence of characters contained in \a chars */
template <class T>
std::pair<script_iterator<typename T::const_iterator>,
    script_iterator<typename T::const_iterator>>
make_script_iterator(const T& chars)
{
    return {script_iterator(chars.cbegin()), script_iterator(chars.cend())};
}

/*** rule_base ***************************************************************/

template <class Rule, class Ctx, class Self, class Up, class Info,
    std::forward_iterator It, handler<Ctx, Self, Up, Info, It> Handler>
auto rule_base<Rule, Ctx, Self, Up, Info, It, Handler>::parse(
                    Ctx& ctx, Up* up, It begin, It end,
                    std::optional<It>& e_it) const -> parse_result
{
    if (ctx.max_depth && ctx.depth >= ctx.max_depth)
        throw error(begin, ctx.depth_msg);
    finally dec_depth{[&d = ctx.depth]() noexcept { --d; }};
    ++ctx.depth;
    if (ctx.trace && !trace.empty()) {
        if constexpr (is_script_iterator_v<It>)
            ctx.trace(std::nullopt, trace, ctx.depth, "",
                      begin.line, begin.column, end.line, end.column,
                      std::nullopt, std::nullopt);
        else
            ctx.trace(std::nullopt, trace, ctx.depth, "", 0, 0, 0, 0,
                      std::nullopt, std::nullopt);
    }
    Self self{};
    auto saved_msg = ctx.error_msg;
    if (!error_msg.empty())
        ctx.error_msg = error_msg;
    parse_result result =
        static_cast<const Rule*>(this)->parse_internal(ctx, self, up,
                                                       begin, end, e_it);
    if (result.first == rule_result::ok ||
        result.first == rule_result::ok_final)
    {
        e_it.reset();
        ctx.error_msg = saved_msg;
    } else
        if (!e_it)
            e_it = result.second;
    if (ctx.trace && !trace.empty()) {
        std::string_view error =
            ctx.error_msg && result.first != rule_result::ok &&
            result.first != rule_result::ok_final ?
                *ctx.error_msg : "";
        if constexpr (is_script_iterator_v<It>) {
            auto [b, e] =
                result.first == rule_result::ok ||
                result.first == rule_result::ok_final ?
                std::make_pair(begin, result.second) :
                std::make_pair(result.second, end);
            ctx.trace(result.first, trace, ctx.depth, error,
                      b.line, b.column, e.line, e.column,
                      e_it ? std::make_optional(e_it->line) : std::nullopt,
                      e_it ? std::make_optional(e_it->column) : std::nullopt);
        } else
            ctx.trace(result.first, trace, ctx.depth, error, 0, 0, 0, 0,
                      std::nullopt, std::nullopt);
    }
    return result;
}

//! This namespace contains various reusable parser rules
/*! Templates of parent rules that have child rules (e.g., rules::repeat,
 * rules::seq, rules::alt, rules::cut) can accept an rvalue as well as an
 * lvalue reference to a child node. If rvalue is used, then a copy of the
 * child rule is stored in the parent rule and remains valid for the lifetime
 * of the parent rule. If lvalue is used, then a reference to the child rule is
 * stored in the parent rule. The caller must manage lifetimes of the parent
 * and the child rule so that the parent's reference to the child does not
 * become dangling.
 * \test in file test_parser.cpp */
namespace rules {

//! A rule that always fails.
/*! \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    class Handler = default_handler<Ctx, Self, Up, empty, It>>
class fail final:
    public rule_base<fail<Ctx, Self, Up, It, Handler>,
        Ctx, Self, Up, empty, It, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<fail, Ctx, Self, Up, empty, It, Handler>;
    using base::base;
};

//! A rule that matches the end of input.
/*! This rule does not consume any input, therefore it must not be used in an
 * unlimited rules::repeat, because it would create an endless loop.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    class Handler = default_handler<Ctx, Self, Up, empty, It>>
class eof final:
    public rule_base<eof<Ctx, Self, Up, It, Handler>,
        Ctx, Self, Up, empty, It, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<eof, Ctx, Self, Up, empty, It, Handler>;
    using base::base;
protected:
    //! \copydoc rule_base::eval()
    typename eof::parse_result eval([[maybe_unused]] Ctx& ctx,
                                [[maybe_unused]] Self& self,
                                [[maybe_unused]] empty& tmp,
                                It begin, It end,
                                [[maybe_unused]] std::optional<It>& e_it) const
    {
        return {begin == end ? rule_result::ok : rule_result::fail, begin};
    }
    //! rule_base needs access to overriden member functions
    friend base;
};

//! A rule that matches any single terminal symbol.
/*! \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    class Handler = default_handler<Ctx, Self, Up, empty, It>>
class any final:
    public rule_base<any<Ctx, Self, Up, It, Handler>,
        Ctx, Self, Up, empty, It, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<any, Ctx, Self, Up, empty, It, Handler>;
    using base::base;
    //! Creates the rule
    /*! \param[in] out a place where a matched terminal will be stored */
    explicit any(typename any::term_type& out):
        any([&out](auto&&, auto&&, auto&&, auto&&, auto&& it, auto&&) {
                out = *it;
            }) {}
protected:
    //! \copydoc rule_base::eval()
    typename any::parse_result eval([[maybe_unused]] Ctx& ctx,
                                [[maybe_unused]] Self& self,
                                [[maybe_unused]] empty& tmp,
                                It begin, It end,
                                [[maybe_unused]] std::optional<It>& e_it) const
    {
        if (begin != end)
            return {rule_result::ok, ++begin};
        else
            return {rule_result::fail, begin};
    }
    //! rule_base needs access to overriden member functions
    friend base;
};

//! A rule that matches a specific single terminal symbol.
/*! \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    class Handler = default_handler<Ctx, Self, Up, empty, It>>
class t final: public rule_base<t<Ctx, Self, Up, It, Handler>,
    Ctx, Self, Up, empty, It, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<t<Ctx, Self, Up, It, Handler>,
        Ctx, Self, Up, empty, It, Handler>;
    //! Creates the rule with a handler.
    /*! \param[in] term the terminal symbol matched by this rule
     * \param[in] hnd the handler stored in the rule */
    explicit t(typename t::term_type term, Handler hnd = {}):
        base(std::move(hnd)), term(term) {}
    //! Creates the rule
    /*! \param[in] term the terminal symbol matched by this rule
     * \param[in] out a place where a matched terminal will be stored */
    explicit t(typename t::term_type term, typename t::term_type& out):
        t(term,
          [&out](auto&&, auto&&, auto&&, auto&&, auto&& it, auto&&) {
              out = *it;
          }) {}
protected:
    //! \copydoc rule_base::eval()
    typename t::parse_result eval([[maybe_unused]] Ctx& ctx,
                              [[maybe_unused]] Self& self,
                              [[maybe_unused]] empty& tmp,
                              It begin, It end,
                              [[maybe_unused]] std::optional<It>& e_it) const
    {
        if (begin != end && *begin == term)
                return {rule_result::ok, ++begin};
        else
            return {rule_result::fail, begin};
    }
private:
    typename t::term_type term; //!< The terminal symbol matched by this rule
    //! rule_base needs access to overriden member functions
    friend base;
};

//! A rule that matches a terminal symbol for which a predicate returns \c true
/*! \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Predicate a predicate for testing if a symbol matches
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    predicate<It> Predicate,
    class Handler = default_handler<Ctx, Self, Up, empty, It>>
class p final: public rule_base<p<Ctx, Self, Up, It, Predicate, Handler>,
    Ctx, Self, Up, empty, It, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<p<Ctx, Self, Up, It, Predicate, Handler>,
        Ctx, Self, Up, empty, It, Handler>;
    //! The type of a predicate
    using predicate_type = Predicate;
    //! Creates the rule with a handler.
    /*! \param[in] pred the predicate for testing input
     * \param[in] hnd the handler stored in the rule */
    explicit p(Predicate pred, Handler hnd = {}):
        base(std::move(hnd)), pred(std::move(pred)) {}
    //! Creates the rule.
    /*! \param[in] pred the predicate for testing input
     * \param[in] out a place where a matched terminal will be stored */
    explicit p(Predicate pred, typename p::term_type& out):
        p(std::move(pred),
          [&out](auto&&, auto&&, auto&&, auto&&, auto&& it, auto&&) {
              out = *it;
          }) {}
protected:
    //! \copydoc rule_base::eval()
    typename p::parse_result eval([[maybe_unused]] Ctx& ctx,
                              [[maybe_unused]] Self& self,
                              [[maybe_unused]] empty& tmp,
                              It begin, It end,
                              [[maybe_unused]] std::optional<It>& e_it) const
    {
        if (begin != end && pred(*begin))
            return {rule_result::ok, ++begin};
        else
            return {rule_result::fail, begin};
    }
private:
    Predicate pred; //!< The predicate for testing input
    //! rule_base needs access to overriden member functions
    friend base;
};

//! Creates a rules::p object
/*! It deduces type \a Predicate from parameter \a pred, but does not deduce a
 * handler type and converts \a hnd to default_handler.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Predicate a predicate for testing if a symbol matches
 * \param[in] pred the predicate to be used by the created rule
 * \param[in] hnd the handler stored in the rule
 * \return a rules::p object */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    predicate<It> Predicate>
auto make_p(Predicate pred, default_handler<Ctx, Self, Up, empty, It> hnd = {})
{
    return p<Ctx, Self, Up, It, Predicate>(std::move(pred), std::move(hnd));
}

//! Creates a rules::p object
/*! It deduces type \a Predicate from parameter \a pred.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Predicate a predicate for testing if a symbol matches
 * \param[in] pred the predicate to be used by the created rule
 * \param[in] out a place where a matched terminal will be stored
 * \return a rules::p object */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    predicate<It> Predicate>
auto make_p(Predicate pred, typename std::iterator_traits<It>::value_type& out)
{
    return p<Ctx, Self, Up, It, Predicate>(std::move(pred), out);
}

//! Creates a rules::p object
/*! It deduces both types \a Predicate from parameter \a pred and \a Handler
 * from parameter \a pred.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches
 * \tparam Predicate a predicate for testing if a symbol matches
 * \param[in] pred the predicate to be used by the created rule
 * \param[in] hnd the handler stored in the rule
 * \return a rules::p object */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    class Handler, predicate<It> Predicate>
auto make_p_hnd(Predicate pred, Handler hnd)
{
    return p<Ctx, Self, Up, It, Predicate, Handler>(std::move(pred),
                                                    std::move(hnd));
}

//! A rule that matches a sequence from input
/*! It tests that a predicate returns \c true for corresponding symbols from
 * the input and from the stored sequence.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Seq a sequence of terminal symbols
 * \tparam Predicate a predicate for testing if a symbol matches
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    class Seq,
    predicate2<It> Predicate =
        std::equal_to<typename std::iterator_traits<It>::value_type>,
    class Handler = default_handler<Ctx, Self, Up, empty, It>>
    requires requires (Seq s) { std::begin(s); std::end(s); }
class str final:
    public rule_base<str<Ctx, Self, Up, It, Seq, Predicate, Handler>,
        Ctx, Self, Up, empty, It, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<str<Ctx, Self, Up, It, Seq, Predicate, Handler>,
        Ctx, Self, Up, empty, It, Handler>;
    //! The type of a predicate
    using predicate_type = Predicate;
    //! Creates the rule with a handler.
    /*! \param[in] seq a sequence of symbols to be stored and matched to input
     * \param[in] pred the predicate for testing input
     * \param[in] hnd the handler stored in the rule */
    explicit str(Seq seq, Predicate pred = {}, Handler hnd = {}):
        base(std::move(hnd)), seq(std::move(seq)), pred(std::move(pred)) {}
protected:
    //! \copydoc rule_base::eval()
    typename str::parse_result eval([[maybe_unused]] Ctx& ctx,
                                [[maybe_unused]] Self& self,
                                [[maybe_unused]] empty& tmp,
                                It begin, It end,
                                [[maybe_unused]] std::optional<It>& e_it) const
    {
        for (auto& s: seq) {
            if (begin == end || !pred(s, *begin))
                return {rule_result::fail, begin};
            ++begin;
        }
        return {rule_result::ok, begin};
    }
private:
    Seq seq; //!< The stored sequence
    Predicate pred; //!< Predicate for comparing \ref str and symbols from input
    //! rule_base needs access to overriden member functions
    friend base;
};

//! Creates a rules::str object
/*! It deduces type \a Seq from parameter \a seq.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Seq a sequence of terminal symbols
 * \param[in] seq a sequence of symbols to be stored and matched to input
 * \return a rules::p object */
template <class Ctx, class Self, class Up, std::forward_iterator It, class Seq>
auto make_str(Seq seq)
{
    return str<Ctx, Self, Up, It, Seq>(std::move(seq));
}

//! Creates a rules::str object
/*! It deduces types \a Seq and \a Predicate from parameters \a seq and \a
 * pred, but does not deduce a handler type and converts \a hnd to
 * default_handler.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Seq a sequence of terminal symbols
 * \tparam Predicate a predicate for testing if a symbol matches
 * \param[in] seq a sequence of symbols to be stored and matched to input
 * \param[in] pred the predicate to be used by the created rule
 * \param[in] hnd the handler stored in the rule
 * \return a rules::p object */
template <class Ctx, class Self, class Up, std::forward_iterator It, class Seq,
    predicate2<It> Predicate>
auto make_str(Seq seq, Predicate pred,
              default_handler<Ctx, Self, Up, empty, It> hnd = {})
{
    return str<Ctx, Self, Up, It, Seq, Predicate>(std::move(seq),
                                              std::move(pred), std::move(hnd));
}

//! Creates a rules::str object
/*! It deduces types \a Seq, \a Predicate, and \a Handler from parameters
 * \a seq, \a pred, and \a handler.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches
 * \tparam Seq a sequence of terminal symbols
 * \tparam Predicate a predicate for testing if a symbol matches
 * \param[in] seq a sequence of symbols to be stored and matched to input
 * \param[in] pred the predicate to be used by the created rule
 * \param[in] hnd the handler stored in the rule
 * \return a rules::p object */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    class Handler, class Seq, predicate2<It> Predicate =
        std::equal_to<typename std::iterator_traits<It>::value_type>>
auto make_str_hnd(Seq seq, Predicate pred, Handler hnd)
{
    return str<Ctx, Self, Up, It, Seq, Predicate, Handler>(std::move(seq),
                                               std::move(pred), std::move(hnd));
}

namespace impl {

//! A child rule type with removed reference, \c const, \c volatile
/*! \tparam R a child rule type */
template <rule_cvref R> using child_t = std::remove_cvref_t<R>;

//! A rule context type
/*! \tparam R a rule type */
template <rule_cvref R> using context_t = typename child_t<R>::context_type;

//! A rule temporary context
/*! \tparam R a rule type */
template <rule_cvref R> using self_ctx_t = typename child_t<R>::self_ctx_type;

//! A rule parent temporary context
/*! \tparam R a rule type */
template <rule_cvref R> using up_ctx_t = typename child_t<R>::up_ctx_type;

//! A rule iterator type
/*! \tparam R a rule type */
template <rule_cvref R> using iterator_t = typename child_t<R>::iterator_type;

//! A rule handler type
/*! \tparam R a rule type */
template <rule_cvref R> using handler_t = typename child_t<R>::handler_type;

} // namespace impl

//! A rule that matches some number of occurrences of a child rule
/*! Parameter \a info of \a Handler has type \c size_t and contains the number
 * of matches of the child rule.
 * \tparam Child a child rule type
 * \tparam Up a temporary context of a parent rule
 * \tparam Handler the type of a handler called when the rule matches */
template <rule_cvref Child, class Up,
    class Handler = default_handler<impl::context_t<Child>,
        impl::up_ctx_t<Child>, Up, size_t, impl::iterator_t<Child>>>
class repeat final: public rule_base<repeat<Child, Up, Handler>,
    impl::context_t<Child>, impl::up_ctx_t<Child>, Up, size_t,
    impl::iterator_t<Child>, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<repeat<Child, Up, Handler>, impl::context_t<Child>,
        impl::up_ctx_t<Child>, Up, size_t, impl::iterator_t<Child>, Handler>;
    //! The type of the child rule
    using child_type = Child;
    //! It denotes an unlimited maximum number of occurrences of the child node
    static constexpr size_t unlimited = 0;
    //! Creates the rule with a handler.
    /*! \param[in] child the child node
     * \param[in,out] up a temporary context of a parent rule; it is a dummy
     * argument, usually created by up_null()
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches
     * \param[in] hnd the handler stored in the rule
     * \note Template magic ensures that \a Handler is not deduced to an
     * integral type when called without an explicit \a Handler, e.g., from
     * \c operator-(). */
    template <class = void> requires (!std::convertible_to<Handler, size_t>)
    explicit repeat(Child child, [[maybe_unused]] Up* up,
                    size_t min = 0, size_t max = unlimited, Handler hnd = {}):
        base(std::move(hnd)), _child(std::forward<Child>(child)), min(min),
        max(max) {}
    //! Creates the rule with a handler.
    /*! \param[in] child the child node
     * \param[in,out] up a temporary context of a parent rule; it is a dummy
     * argument, usually created by up_null()
     * \param[in] hnd the handler stored in the rule
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches
     * \note Template magic ensures that \a Handler is not deduced to an
     * integral type when called without an explicit \a Handler. */
    template <class = void> requires (!std::convertible_to<Handler, size_t>)
    repeat(Child child, [[maybe_unused]] Up* up, Handler hnd,
           size_t min = 0, size_t max = unlimited):
        repeat(std::forward<Child>(child), min, max, hnd) {}
    //! Creates the rule.
    /*! \param[in] child the child rule
     * \param[in,out] up a temporary context of a parent rule; it is a dummy
     * argument, usually created by up_null()
     * \param[in] out a place where the number of matches will be stored if the
     * child rule matches at least \a min times
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches
     * maches between \ref min and \ref max times */
    repeat(Child child, [[maybe_unused]] Up* up, size_t& out,
           size_t min = 0, size_t max = unlimited):
        repeat(std::forward<Child>(child),
               [&out](auto&&, auto&&, auto&&, size_t info, auto&&, auto&&) {
                   out = info;
               },
               min, max) {}
    //! Gets the child node.
    /*! \return the child */
    const Child& child() const noexcept { return _child; }
protected:
    //! \copydoc rule_base::parse_internal()
    typename repeat::parse_result
    parse_internal(typename repeat::context_type& ctx,
                   typename repeat::self_ctx_type& self,
                   typename repeat::up_ctx_type* up,
                   typename repeat::iterator_type begin,
                   typename repeat::iterator_type end,
                   std::optional<typename repeat::iterator_type>& e_it) const
    {
        size_t tmp = 0;
        return this->parse_with_tmp(ctx, self, up, tmp, begin, end, e_it);
    }
    //! \copydoc rule_base::eval()
    typename repeat::parse_result eval(typename repeat::context_type& ctx,
                   typename repeat::self_ctx_type& self,
                   size_t& tmp,
                   typename repeat::iterator_type begin,
                   typename repeat::iterator_type end,
                   std::optional<typename repeat::iterator_type>& e_it) const
    {
        for (size_t i = 0;; ++i) {
            switch (auto [result, pos] = _child.parse(ctx, &self, begin, end,
                                                      e_it); result)
            {
            case rule_result::fail:
            case rule_result::fail_final_seq:
            case rule_result::fail_final_alt:
                if (i >= min) {
                    tmp = i;
                    return {rule_result::ok, begin};
                } else
                    return {rule_result::fail, pos};
            case rule_result::ok:
            case rule_result::ok_final:
                if (max != unlimited && i >= max - 1) {
                    tmp = i + 1;
                    return {rule_result::ok, pos};
                }
                begin = pos;
                break;
            default:
                assert(false);
            }
        }
    }
private:
    Child _child; //!< The child node
    size_t min = 0; //!< The minimum number of matches of the child node
    size_t max = unlimited; //!< The maximum number of matches of the child node
    //! rule_base needs access to overriden member functions
    friend base;
};

//! A deduction guide for \ref repeat
/*! \tparam Child a child rule type
 * \tparam Up a temporary context of a parent rule
 * \note \a Up is usually created by up_null(). */
template <class Child, class Up>
repeat(Child&&, Up*, size_t = {}, size_t = {}) -> repeat<Child, Up>;

//! A deduction guide for \ref repeat
/*! \tparam Child a child rule type
 * \tparam Up a temporary context of a parent rule
 * \tparam Handler a handler type
 * \note \a Up is usually created by up_null(). */
template <class Child, class Up, class Handler>
    requires (!std::convertible_to<Handler, size_t>)
repeat(Child&&, Up*, size_t, size_t, Handler) -> repeat<Child, Up, Handler>;

//! A deduction guide for \ref repeat
/*! \tparam Child a child rule type
 * \tparam Up a temporary context of a parent rule
 * \tparam Handler a handler type
 * \note \a Up is usually created by up_null(). */
template <class Child, class Up, class Handler>
    requires (!std::convertible_to<Handler, size_t>)
repeat(Child&&, Up*, Handler, size_t = {}, size_t = {}) ->
    repeat<Child, Up, Handler>;

//! A deduction guide for \ref repeat
/*! \tparam Child a child rule type
 * \tparam Up a temporary context of a parent rule
 * \tparam Handler a handler type
 * \note \a Up is usually created by up_null(). */
template <class Child, class Up>
repeat(Child&&, Up*, size_t&, size_t = {}, size_t = {}) -> repeat<Child, Up>;

//! A rule that matches a sequential composition of two child rules
/*! It matches iff the first child matches an initial part of the input and the
 * second child matches an immediately following part of the input.
 * \tparam Child1 the first child rule type
 * \tparam Child2 the second child rule type
 * \tparam Up a temporary context of a parent rule
 * \tparam Handler the type of a handler called when the rule matches */
template <rule_cvref Child1, rule_cvref Child2, class Up,
    class Handler = default_handler<impl::context_t<Child1>,
        impl::up_ctx_t<Child1>, Up, empty, impl::iterator_t<Child1>>>
    requires
        std::same_as<impl::context_t<Child1>, impl::context_t<Child2>> &&
        std::same_as<impl::up_ctx_t<Child1>, impl::up_ctx_t<Child2>> &&
        std::same_as<impl::iterator_t<Child1>, impl::iterator_t<Child2>>
class seq final: public rule_base<seq<Child1, Child2, Up, Handler>,
    impl::context_t<Child1>, impl::up_ctx_t<Child1>, Up, empty,
    impl::iterator_t<Child1>, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<seq<Child1, Child2, Up, Handler>,
        impl::context_t<Child1>, impl::up_ctx_t<Child1>, Up, empty,
        impl::iterator_t<Child1>, Handler>;
    //! The type of the first child rule
    using child1_type = Child1;
    //! The type of the second child rule
    using child2_type = Child2;
    //! Creates the rule with a handler.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in,out] up a temporary context of a parent rule; it is a dummy
     * argument, usually created by up_null()
     * \param[in] hnd the handler stored in the rule */
    seq(Child1 child1, Child2 child2, [[maybe_unused]] Up* up,
        Handler hnd = {}):
        base(std::move(hnd)), _child1(std::forward<Child1>(child1)),
        _child2(std::forward<Child2>(child2)) {}
    //! Creates the rule.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in,out] up a temporary context of a parent rule; it is a dummy
     * argument, usually created by up_null()
     * \param[in] out a place where \c true will be stored if the sequence of
     * child rules matches */
    seq(Child1 child1, Child2 child2, [[maybe_unused]] Up* up, bool& out):
        seq(std::forward<Child1>(child1), std::forward<Child2>(child2),
            [&out](auto&&, auto&&, auto&&, auto&&, auto&&, auto&&) {
                out = true;
            }) {}
    //! Gets the first child node.
    /*! \return the child */
    const Child1& child1() const noexcept { return _child1; }
    //! Gets the second child node.
    /*! \return the child */
    const Child2& child2() const noexcept { return _child2; }
protected:
    //! \copydoc rule_base::eval()
    typename seq::parse_result eval(typename seq::context_type& ctx,
                        typename seq::self_ctx_type& self,
                        [[maybe_unused]] empty& tmp,
                        typename seq::iterator_type begin,
                        typename seq::iterator_type end,
                        std::optional<typename seq::iterator_type>& e_it) const
    {
        switch (auto [result1, pos1] = _child1.parse(ctx, &self, begin, end,
                                                     e_it); result1)
        {
        case rule_result::fail_final_alt:
            result1 = rule_result::fail;
            [[fallthrough]];
        case rule_result::fail:
        case rule_result::fail_final_seq:
            return {result1, pos1};
        case rule_result::ok:
        case rule_result::ok_final:
            switch (auto [result2, pos2] = _child2.parse(ctx, &self, pos1, end,
                                                         e_it); result2)
            {
            case rule_result::fail_final_alt:
                result2 = rule_result::fail;
                [[fallthrough]];
            case rule_result::fail:
            case rule_result::fail_final_seq:
                if (result1 == rule_result::ok_final)
                    result2 = rule_result::fail_final_seq;
                return {result2, pos2};
            case rule_result::ok:
            case rule_result::ok_final:
                if (result1 == rule_result::ok_final)
                    result2 = rule_result::ok_final;
                return {result2, pos2};
            default:
                assert(false);
            }
        default:
            assert(false);
        }
    }
private:
    Child1 _child1; //!< The first child node
    Child2 _child2; //!< The second child node
    //! rule_base needs access to overriden member functions
    friend base;
};

//! A deduction guide for \ref seq
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type
 * \tparam Up a temporary context of a parent rule
 * \note \a Up is usually created by up_null(). */
template <class Child1, class Child2, class Up>
seq(Child1&&, Child2&&, Up*) -> seq<Child1, Child2, Up>;

//! A deduction guide for \ref seq
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type
 * \tparam Up a temporary context of a parent rule
 * \tparam Handler a handler type
 * \note \a Up is usually created by up_null(). */
template <class Child1, class Child2, class Up, class Handler>
seq(Child1&&, Child2&&, Up*, Handler) -> seq<Child1, Child2, Up, Handler>;

//! A deduction guide for \ref seq
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type
 * \tparam Up a temporary context of a parent rule
 * \note \a Up is usually created by up_null(). */
template <class Child1, class Child2, class Up>
seq(Child1&&, Child2&&, Up*, bool&) -> seq<Child1, Child2, Up>;

//! A rule that matches an alternative of two child rules
/*! It matches iff the first or the second child matches. If the first child
 * matches, matching of the second child is not tried.
 * Parameter \a info of \a Handler has type \c size_t and contains the number
 * of matched child rule (1 or 2).
 * \tparam Child1 the first child rule type
 * \tparam Child2 the second child rule type
 * \tparam Up a temporary context of a parent rule
 * \tparam Handler the type of a handler called when the rule matches */
template <rule_cvref Child1, rule_cvref Child2, class Up,
    class Handler = default_handler<impl::context_t<Child1>,
        impl::up_ctx_t<Child1>, Up, size_t, impl::iterator_t<Child1>>>
    requires
        std::same_as<impl::context_t<Child1>, impl::context_t<Child2>> &&
        std::same_as<impl::up_ctx_t<Child1>, impl::up_ctx_t<Child2>> &&
        std::same_as<impl::iterator_t<Child1>, impl::iterator_t<Child2>>
class alt final: public rule_base<alt<Child1, Child2, Up, Handler>,
    impl::context_t<Child1>, impl::up_ctx_t<Child1>, Up, size_t,
    impl::iterator_t<Child1>, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<alt<Child1, Child2, Up, Handler>,
        impl::context_t<Child1>, impl::up_ctx_t<Child1>, Up, size_t,
        impl::iterator_t<Child1>, Handler>;
    //! The type of the first child rule
    using child1_type = Child1;
    //! The type of the second child rule
    using child2_type = Child2;
    //! Creates the rule with a handler.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in,out] up a temporary context of a parent rule; it is a dummy
     * argument, usually created by up_null()
     * \param[in] hnd the handler stored in the rule */
    alt(Child1 child1, Child2 child2, [[maybe_unused]] Up* up,
        Handler hnd = {}):
        base(std::move(hnd)), _child1(std::forward<Child1>(child1)),
        _child2(std::forward<Child2>(child2)) {}
    //! Creates the rule.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in,out] up a temporary context of a parent rule; it is a dummy
     * argument, usually created by up_null()
     * \param[in] out a place where the number of the the matched child (1 or
     * 2) will be stored if either of the alternative child rules matches */
    alt(Child1 child1, Child2 child2, [[maybe_unused]] Up* up, size_t& out):
        alt(std::forward<Child1>(child1), std::forward<Child2>(child2),
            [&out](auto&&, auto&&, auto&&, size_t info, auto&&, auto&&) {
                out = info;
            })
    {}
    //! Gets the first child node.
    /*! \return the child */
    const Child1& child1() const noexcept { return _child1; }
    //! Gets the second child node.
    /*! \return the child */
    const Child2& child2() const noexcept { return _child2; }
protected:
    //! \copydoc rule_base::parse_internal()
    typename alt::parse_result
    parse_internal(typename alt::context_type& ctx,
                   typename alt::self_ctx_type& self,
                   typename alt::up_ctx_type* up,
                   typename alt::iterator_type begin,
                   typename alt::iterator_type end,
                   std::optional<typename alt::iterator_type>& e_it) const
    {
        size_t tmp = 2;
        return this->parse_with_tmp(ctx, self, up, tmp, begin, end, e_it);
    }
    //! \copydoc rule_base::eval()
    typename alt::parse_result eval(typename alt::context_type& ctx,
                        typename alt::self_ctx_type& self,
                        size_t& tmp,
                        typename alt::iterator_type begin,
                        typename alt::iterator_type end,
                        std::optional<typename alt::iterator_type>& e_it) const
    {
        switch (auto [result1, pos1] = _child1.parse(ctx, &self, begin, end,
                                                     e_it); result1)
        {
        case rule_result::fail_final_seq:
        case rule_result::fail_final_alt:
            return {rule_result::fail_final_alt, pos1};
        case rule_result::fail:
            switch (auto [result2, pos2] = _child2.parse(ctx, &self, begin,
                                                         end, e_it); result2)
            {
            case rule_result::fail_final_seq:
                result2 = rule_result::fail_final_alt;
                [[fallthrough]];
            case rule_result::fail_final_alt:
            case rule_result::fail:
                return {result2, pos2};
            case rule_result::ok:
            case rule_result::ok_final:
                return {rule_result::ok, pos2};
            default:
                assert(false);
            }
        case rule_result::ok:
        case rule_result::ok_final:
            tmp = 1;
            return {rule_result::ok, pos1};
        default:
            assert(false);
        }
    }
private:
    Child1 _child1; //!< The first child node
    Child2 _child2; //!< The second child node
    //! rule_base needs access to overriden member functions
    friend base;
};

//! A deduction guide for \ref alt
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type
 * \tparam Up a temporary context of a parent rule
 * \note \a Up is usually created by up_null(). */
template <class Child1, class Child2, class Up>
alt(Child1&&, Child2&&, Up*) -> alt<Child1, Child2, Up>;

//! A deduction guide for \ref alt
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type
 * \tparam Up a temporary context of a parent rule
 * \note \a Up is usually created by up_null().
 * \tparam Handler a handler type */
template <class Child1, class Child2, class Up, class Handler>
alt(Child1&&, Child2&&, Up*, Handler) -> alt<Child1, Child2, Up, Handler>;

//! A deduction guide for \ref alt
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type
 * \tparam Up a temporary context of a parent rule
 * \note \a Up is usually created by up_null(). */
template <class Child1, class Child2, class Up>
alt(Child1&&, Child2&&, Up*, size_t&) -> alt<Child1, Child2, Up>;

//! A rule that disables (cuts) the following alternatives in rules::alt
/*! If this node is a member of a sequence of rule::seq nodes that is a child
 * of a rule::alt node, than after this node  matches, the following
 * alternatives in the sequence of rules::alt nodes will not be matched,
 * regardless the outcome of the rest of rule::seq sequence after this node.
 * Eliminating alternatives works only if this node is a direct child of a
 * rules::seq node and there are only rules::seq nodes on the path to the
 * nearest rules::alt ancestor.
 * \tparam Child a child rule type
 * \tparam Up a temporary context of a parent rule */
template <rule_cvref Child, class Up>
class cut final: public rule_base<cut<Child, Up>, impl::context_t<Child>,
    impl::up_ctx_t<Child>, Up, empty, impl::iterator_t<Child>,
    impl::handler_t<Child>>
{
public:
    //! The alias for the base class
    using base = rule_base<cut<Child, Up>, impl::context_t<Child>,
        impl::up_ctx_t<Child>, Up, empty, impl::iterator_t<Child>,
        impl::handler_t<Child>>;
    //! The type of the child rule
    using child_type = Child;
    //! Creates the rule
    /*! \param[in] child the child node
     * \param[in,out] up a temporary context of a parent rule; it is a dummy
     * argument, usually created by up_null() */
    explicit cut(Child child, [[maybe_unused]] Up* up):
        base(), _child(std::forward<Child>(child)) {}
    //! Gets the child node.
    /*! \return the child */
    const Child& child() const noexcept { return _child; }
protected:
    //! \copydoc rule_base::eval()
    typename cut::parse_result eval(typename cut::context_type& ctx,
                        typename cut::self_ctx_type& self,
                        [[maybe_unused]] empty& tmp,
                        typename cut::iterator_type begin,
                        typename cut::iterator_type end,
                        std::optional<typename cut::iterator_type>& e_it) const
    {
        switch (auto [result, pos] = _child.parse(ctx, &self, begin, end, e_it);
                result)
        {
        case rule_result::fail:
        case rule_result::fail_final_seq:
        case rule_result::fail_final_alt:
            return {rule_result::fail, pos};
        case rule_result::ok:
        case rule_result::ok_final:
            return {rule_result::ok_final, pos};
        default:
            assert(false);
        }
    }
private:
    Child _child; //!< The child node
    //! rule_base needs access to overriden member functions
    friend base;
};

//! A deduction guide for \ref cut
/*! \tparam Child a child rule type
 * \tparam Up a temporary context of a parent rule
 * \note \a Up is usually created by up_null(). */
template <class Child, class Up>
cut(Child&&, Up*) -> cut<Child, Up>;

//! A rule that provides dynamic replacement of a child rule.
/*! It is intended for defining recursive rules.
 * \tparam Ctx a parsing context
 * \tparam Self a temporary context of this rule
 * \tparam Up a temporary context of a parent rule
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, class Self, class Up, std::forward_iterator It,
    class Handler = default_handler<Ctx, Self, Up, empty, It>>
class dyn final: public rule_base<dyn<Ctx, Self, Up, It, Handler>, Ctx, Self,
    Up, empty, It, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<dyn<Ctx, Self, Up, It, Handler>, Ctx, Self, Up,
        empty, It, Handler>;
    using base::base;
    //! Sets the child node.
    /*! \tparam Rule the type of the child node
     * \param[in] r the child node */
    template <rule_cvref Rule> requires
        std::same_as<impl::context_t<Rule>, Ctx> &&
        std::same_as<impl::up_ctx_t<Rule>, Self> &&
        std::same_as<impl::iterator_t<Rule>, It> &&
        std::is_lvalue_reference_v<Rule>
    void set_child(Rule&& r) {
        _child = &r;
        _parse = [](const void* child, Ctx& ctx, Up* up, It begin, It end,
                    std::optional<It>& e_it) {
            return static_cast<const std::remove_cvref_t<Rule>*>(child)->
                parse(ctx, up, begin, end, e_it);
        };
    }
    /*! \copydoc set_child()
     * \return \c *this */
    template <rule_cvref Rule> dyn& operator>>=(Rule&& r) & {
        set_child(std::forward<Rule>(r));
        return *this;
    }
    /*! \copydoc set_child()
     * \return \c *this */
    template <rule_cvref Rule> dyn operator>>=(Rule&& r) && {
        set_child(std::forward<Rule>(r));
        return std::move(*this);
    }
    //! Tests if a child node has been set.
    /*! \return \c true if a child node has been set, \c false otherwise */
    [[nodiscard]] bool has_child() const noexcept { return _child && _parse; }
    //! Tests if a child node has been set.
    /*! \return the result of has_child() */
    explicit operator bool() const noexcept { return has_child(); }
protected:
    //! \copydoc rule_base::eval()
    typename dyn::parse_result eval(Ctx& ctx, Self& self,
                                    [[maybe_unused]] empty& tmp,
                                    It begin, It end,
                                    std::optional<It>& e_it) const
    {
        if (_child && _parse)
            switch(auto [result, pos] = _parse(_child, ctx, &self, begin, end,
                                               e_it); result)
            {
            case rule_result::fail:
            case rule_result::fail_final_seq:
            case rule_result::fail_final_alt:
                return {rule_result::fail, pos};
            case rule_result::ok:
            case rule_result::ok_final:
                return {rule_result::ok, pos};
            default:
                assert(false);
            }
        else
            return {rule_result::fail, begin};
    }
private:
    const void* _child = nullptr; //!< Type-erased pointer to a child rule
    //! A function that calls <tt>_child->parse()</tt>
    /*! \param[in] child the value of _child is expected, and it must be the
     * value previously set by child()
     * \param[in] ctx a parsing context
     * \param[in,out] up a temporary context of a parent rule; \c nullptr if
     * this rule has no parent
     * \param[in] begin the start of the input
     * \param[in] end the end of the input
     * \param[in,out] e_it an iterator used to track an error location
     * \return a parsing result returned by the child node */
    typename dyn::parse_result (*_parse)(const void* child, Ctx& ctx, Up* up,
                                         It begin, It end,
                                         std::optional<It>& e_it) = nullptr;
    //! rule_base needs access to overriden member functions
    friend base;
};

} // namespace rules

//! Wraps a rule by rules::repeat for 0 or 1 match.
/*! \tparam Rule a type of the child rule
 * \param[in] r a child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 0 and maximum 1 */
template <rule_cvref Rule>
rules::repeat<Rule, up_type_t<rules::impl::up_ctx_t<Rule>>,
    rebind_rhnd_t<Rule, size_t>>
operator-(Rule&& r)
{
    return rules::repeat{std::forward<Rule>(r),
        up_null<up_type_t<rules::impl::up_ctx_t<Rule>>>(), 0, 1};
}

//! Wraps a rule by rules::repeat for at least 1 match.
/*! \tparam Rule a type of the child rule
 * \param[in] r a child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 1 and maximum unlimited */
template <rule_cvref Rule>
rules::repeat<Rule, up_type_t<rules::impl::up_ctx_t<Rule>>,
    rebind_rhnd_t<Rule, size_t>>
operator+(Rule&& r)
{
    using up_t = up_type_t<rules::impl::up_ctx_t<Rule>>;
    return rules::repeat{std::forward<Rule>(r), up_null<up_t>(),
        1, rules::repeat<Rule, up_t>::unlimited};
}

//! Wraps a rule by rules::repeat for any number of matches.
/*! \tparam Rule the type of the child rule
 * \param[in] r the child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 0 and maximum unlimited */
template <rule_cvref Rule>
rules::repeat<Rule, up_type_t<rules::impl::up_ctx_t<Rule>>,
    rebind_rhnd_t<Rule, size_t>>
operator*(Rule&& r)
{
    using up_t = up_type_t<rules::impl::up_ctx_t<Rule>>;
    return rules::repeat{std::forward<Rule>(r), up_null<up_t>(),
        0, rules::repeat<Rule, up_t>::unlimited};
}

//! Creates a sequential composition of two rules.
/*! \tparam Rule1 the type of the first child rule
 * \tparam Rule2 the type of the second child rule
 * \param[in] r1 the first child rule
 * \param[in] r2 the second child rule
 * \return a rules::seq rule containing \a r1 and \a r2 as child rules */
template <rule_cvref Rule1, rule_cvref Rule2> requires
    std::same_as<up_type_t<rules::impl::up_ctx_t<Rule1>>,
        up_type_t<rules::impl::up_ctx_t<Rule2>>>
rules::seq<Rule1, Rule2, up_type_t<rules::impl::up_ctx_t<Rule1>>,
    rebind_rhnd_t<Rule1, empty>>
operator>>(Rule1&& r1, Rule2&& r2)
{
    return rules::seq{std::forward<Rule1>(r1), std::forward<Rule2>(r2),
        up_null<up_type_t<rules::impl::up_ctx_t<Rule1>>>()};
}

//! Creates an alternative composition of two rules.
/*! \tparam Rule1 the type of the first child rule
 * \tparam Rule2 the type of the second child rule
 * \param[in] r1 the first child rule
 * \param[in] r2 the second child rule
 * \return a rules::alt rule containing \a r1 and \a r2 as child rules */
template <rule_cvref Rule1, rule_cvref Rule2> requires
    std::same_as<up_type_t<rules::impl::up_ctx_t<Rule1>>,
        up_type_t<rules::impl::up_ctx_t<Rule2>>>
rules::alt<Rule1, Rule2,  up_type_t<rules::impl::up_ctx_t<Rule1>>,
    rebind_rhnd_t<Rule1, size_t>>
operator|(Rule1&& r1, Rule2&& r2)
{
    return rules::alt{std::forward<Rule1>(r1), std::forward<Rule2>(r2),
        up_null<up_type_t<rules::impl::up_ctx_t<Rule1>>>()};
}

//! Creates a rule that disables (cuts) the following alternatives in rules::alt
/*! \tparam Rule the type of the child rule
 * \param[in] r the child rule
 * \return a rules::cut rule containing \a r as the child rule. */
template <rule_cvref Rule>
rules::cut<Rule, up_type_t<rules::impl::up_ctx_t<Rule>>>
operator!(Rule&& r)
{
    return rules::cut{std::forward<Rule>(r),
        up_null<up_type_t<rules::impl::up_ctx_t<Rule>>>()};
}

} // namespace threadscript::parser
