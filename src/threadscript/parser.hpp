#pragma once

/*! \file
 * \brief A generic recursive descent parser
 *
 * \test in file test_parser.cpp
 */

#include "threadscript/finally.hpp"

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
    explicit error(I pos, const std::string& msg = "Parse error"):
        runtime_error(msg), _pos(pos) {}
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
    /*! It disables the following alternatives. This is normally used by a node
     * to signal to its parent disjunction node that parent's remaining
     * alternatives should not be evaluated. */
    fail_final,
    //! The matches, and an alternative will not be tried.
    /*! It disables the following alternatives. This is normally used by a node
     * to signal to its parent sequence node that if a later node in the
     * sequence fails, the sequence should return fail_final instead of
     * rule_result::fail to its parent disjunction node. */
    ok_final,
};

//! A handler called when a rule matches
/*! A handler gets a context object \a Ctx that has invoked this rule via
 * context::parse(), some information \a Info extracted from the input by the
 * rule (e.g., a numeric value obtained by parsing a sequence of digits), and
 * iterators \a It delimiting the part of the input matched by the rule.
 * \tparam Handler a handler
 * \tparam Ctx a parsing context
 * \tparam Info information about a parsed part of input
 * \tparam It an iterator to the input sequence of terminal symbols */
template <class Handler, class Ctx, class Info, class It> concept handler =
    std::is_invocable_r_v<void, Handler, Ctx&, Info, It, It> &&
    std::forward_iterator<It> && std::derived_from<Ctx, context>;

//! The default handler called when a rule matches
/*! \tparam Ctx a parsing context
 * \tparam Info information about a parsed part of input
 * \tparam It an iterator to the input sequence of terminal symbols
 * \param[in] ctx a parsing context
 * \param[in] info information about a parsed part of input
 * \param[in] begin the first terminal matching a rule
 * \param[in] end one after the last terminal matching a rule */
template <class Ctx, class Info, std::forward_iterator It>
using default_handler =
    std::function<void(Ctx& ctx, Info info, It begin, It end)>;

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

//! A template instance for rebinding default_handler
/*! \tparam Ctx a context type
 * \tparam Info a the original information type
 * \tparam It an iterator type
 * \tparam Info2 a new information type */
template <class Ctx, class Info, class It, class Info2>
struct rebind_handler<default_handler<Ctx, Info, It>, Info2> {
    //! The hew handler type
    using type = default_handler<Ctx, Info2, It>;
};

//! Empty data that can be used as parameter \a Info of a handler.
/*! It is used by default as \a Info if no additional information is produced
 * by a rule, and also as internal temporary data \a Tmp of a rule if the rule
 * does not need any additional data. */
struct empty {};

//! A base class of all rules
/*! Member functions in this class are not virtual, but they can be overriden
 * in a derived class \a Rule, because they are called via \c this pointer
 * static-cast to a pointer to \a Rule.
 *
 * Unless the output type \a Info or the type of temporary private data created
 * by parse_internal() is changed, both these types are \ref empty.
 * \tparam Rule the derived rule class
 * \tparam Ctx a parsing context
 * \tparam Info information about a parsed part of input
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches
 * \threadsafe{safe,unsafe} */
template <class Rule, class Ctx, class Info, std::forward_iterator It,
    handler<Ctx, Info, It> Handler = default_handler<Ctx, Info, It>>
class rule_base {
public:
    using rule_type = Rule; //!< The derived rule type
    //! TThe rule_type without reference, \c const, and \c volatile
    using value_type = std::remove_cvref_t<Rule>;
    using context_type = Ctx; //!< A parsing context
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
    /*! This function consists of sevaral parts, delegated to other member
     * functions:
     * \arg parse_internal() creates temporary private data used during the
     * single call of parse()
     * \arg parse_with_tmp() parses the input by calling eval() and if a match
     * is found, it is further processed by attr()
     * \arg make_info() extracts information about the parsing result from
     * temporary private data into a handler argument
     * \arg attr() calls the registered handler and passes to it a result of
     * parsing
     * \arg parse_internal() destroys temporary private data
     * \arg if the rule does not match, then attr(), make_info(), and the
     * handler are not called
     *
     * If the rule matches, it returns rule_result::ok or rule_result::ok_final
     * and the iterator denoting the end of the matched part of input. If the
     * rule does not match, it returns rule_result::fail or
     * rule_result::fail_final and the position where matching failed.
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \return the result of matching this rule with the input sequence
     * \throw error if the maximum depth of rule nesting is exceeded */
    parse_result parse(Ctx& ctx, It begin, It end) const {
        if (ctx.max_depth && ctx.depth >= ctx.max_depth)
            throw error(begin, ctx.depth_msg);
        finally dec_depth{[&d = ctx.depth]() noexcept { --d; }};
        ++ctx.depth;
        return static_cast<const Rule*>(this)->parse_internal(ctx, begin, end);
    }
    //! Sets a new handler
    /*! \param[in] h the new handler
     * \return \c *this */
    Rule& operator[](Handler h) {
        hnd = std::move(h);
        return static_cast<Rule&>(*this);
    }
    //! The stored handler called by attr().
    /*! The handler gets additional information created by make_info(). */
    Handler hnd{};
protected:
    //! Creates temporary private data usable in this parsing operation.
    /*! Private data are destroyed when the current parsing operation ends.
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \return the result of matching this rule with the input sequence */
    parse_result parse_internal(Ctx& ctx, It begin, It end) const {
        empty tmp{};
        return static_cast<const Rule*>(this)->parse_with_tmp(ctx, tmp,
                                                              begin, end);
    }
    //! Parses the input and can use temporary private data
    /*! When the rules matches, the result is passed out from the rule by
     * calling attr().
     * \tparam Tmp the type of temporary private data
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in,out] tmp temporary private data
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \return the result of matching this rule with the input sequence */
    template <class Tmp>
    parse_result parse_with_tmp(Ctx& ctx, Tmp& tmp, It begin, It end) const {
        auto result =
            static_cast<const Rule*>(this)->eval(ctx, tmp, begin, end);
        if (result.first == rule_result::ok ||
            result.first == rule_result::ok_final)
        {
            static_cast<const Rule*>(this)->attr(ctx, tmp, begin,
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
     * \param[in,out] tmp temporary private data
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \return[in] the result of matching this rule with the input sequence,
     * with the same meaning as in parse() */
    template <class Tmp>
    parse_result eval([[maybe_unused]] Ctx& ctx, [[maybe_unused]] Tmp& tmp,
                      [[maybe_unused]] It begin, [[maybe_unused]] It end) const
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
     * \param[in,out] tmp temporary private data
     * \param[in] begin a position in the input sequence where matching starts;
     * it is the \a begin argument of parse()
     * \param[in] end the end of matching part of the input, as returned by
     * eval()
     * \note It is named \c attr, because its typical functionality is stored
     * a value obtained from the matching part of input into the attribute of
     * an attribute grammar. */
    template <class Tmp>
    void attr(Ctx& ctx, Tmp& tmp, It begin, It end) const {
        if constexpr (requires { bool(hnd); }) {
            if (bool(hnd))
                hnd(ctx, static_cast<const Rule*>(this)->make_info(tmp),
                    begin, end);
        } else
            hnd(ctx, static_cast<const Rule*>(this)->make_info(tmp),
                begin, end);
    }
};

//! A rule, which must be derived from rule_base and have a \c parse() function
/*! \tparam Rule a rule type */
template <class Rule> concept rule = std::derived_from<Rule, rule_base<
    Rule, typename Rule::context_type, typename Rule::info_type,
    typename Rule::iterator_type, typename Rule::handler_type>> &&
    requires (const Rule r, typename Rule::context_type& ctx,
              const typename Rule::iterator_type it)
    {
        { r.parse(ctx, it, it) } -> std::same_as<typename Rule::parse_result>;
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
    //! Parses an input sequence of terminal symbols according to \a rule.
    /*! \tparam R a rule type
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
        depth = 0;
        switch (auto [result, pos] = rule.parse(*this, begin, end); result) {
            case rule_result::fail:
            case rule_result::fail_final:
                if (error_msg)
                    throw error(pos, *error_msg);
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
    /*! \tparam R a rule type
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
        return parse(rule, it.first, it.second, all);
    }
    //! An error message to be stored in an exception.
    /*! If not \c nullopt, then this message is used when a parsing failed
     * exception (\ref error) is thrown, unless a more specific message is
     * used. If \c nullopt, then a default message defined by \ref error is
     * used. */
    std::optional<std::string> error_msg{};
    //! If nont \c nullopt, then it defines the maximum stack depth of parsing.
    std::optional<size_t> max_depth{};
    //! An error message used if max_depth is exceeded
    std::string depth_msg{"Maximum parsing depth exceeded"};
    //! An error message used if only a part of the input has been matched.
    /*! It is used in an exception thrown if a rule matches only a part of the
     * input and argument \a all of parse() is \c true. */
    std::string partial_msg{"Partial match"};
private:
    size_t depth = 0; //!< The current stack depth of parsing
    //! Needed to manipulate context::depth
    template <class Rule, class Ctx, class Info, std::forward_iterator It,
        handler<Ctx, Info, It> Handler> friend class rule_base;
};

//! An iterator for parsing script source code.
/*! It can be used when using any text consisting of a sequence of \c char's.
 * It maintains the current line and column numbers, both starting at 1. It
 * expects lines delimited by <tt>'\\n'</tt>.
 * \tparam It the underlying iterator type, which must point to a \c char */
template <std::forward_iterator It> requires
    requires (It it) { { *it } -> std::same_as<char&>; } ||
    requires (It it) { { *it } -> std::same_as<const char&>; }
class script_iterator: public std::forward_iterator_tag {
public:
    //! A required member of an iterator class
    using difference_type = typename It::difference_type;
    //! A required member of an iterator class
    using value_type = typename It::value_type;
    //! A required member of an iterator class
    using pointer = typename It::pointer;
    //! A required member of an iterator class
    using reference = typename It::reference;
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
        return result;
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
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, empty, It>>
class fail final:
    public rule_base<fail<Ctx, It, Handler>, Ctx, empty, It, Handler>
{
    using rule_base<fail, Ctx, empty, It, Handler>::rule_base;
};

//! A rule that matches the end of input.
/*! This rule does not consume any input, therefore it must not be used in an
 * unlimited rules::repeat, because it would create an endless loop.
 * \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, empty, It>>
class eof final:
    public rule_base<eof<Ctx, It, Handler>, Ctx, empty, It, Handler>
{
    using rule_base<eof, Ctx, empty, It, Handler>::rule_base;
protected:
    //! \copydoc rule_base::eval()
    typename eof::parse_result eval([[maybe_unused]] Ctx& ctx,
                                    [[maybe_unused]] empty& tmp,
                                    It begin, It end) const
    {
        return {begin == end ? rule_result::ok : rule_result::fail, begin};
    }
    //! rule_base needs access to overriden member functions
    friend rule_base<eof, Ctx, empty, It, Handler>;
};

//! A rule that matches any single terminal symbol.
/*! \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, empty, It>>
class any final:
    public rule_base<any<Ctx, It, Handler>, Ctx, empty, It, Handler>
{
    using rule_base<any, Ctx, empty, It, Handler>::rule_base;
public:
    //! Creates the rule
    /*! \param[in] out a place where a matched terminal will be stored */
    explicit any(typename any::term_type& out):
        any([&out](auto&&, auto&&, auto&& it, auto&&) { out = *it; }) {}
protected:
    //! \copydoc rule_base::eval()
    typename any::parse_result eval([[maybe_unused]] Ctx& ctx,
                                    [[maybe_unused]] empty& tmp,
                                    It begin, It end) const
    {
        if (begin != end)
            return {rule_result::ok, ++begin};
        else
            return {rule_result::fail, begin};
    }
    //! rule_base needs access to overriden member functions
    friend rule_base<any, Ctx, empty, It, Handler>;
};

//! A rule that matches a specific single terminal symbol.
/*! \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, empty, It>>
class t final: public rule_base<t<Ctx, It, Handler>, Ctx, empty, It, Handler> {
public:
    //! The alias for the base class
    using base = rule_base<t<Ctx, It, Handler>, Ctx, empty, It, Handler>;
    //! Creates the rule with a handler.
    /*! \param[in] term the terminal symbol matched by this rule
     * \param[in] hnd the handler stored in the rule */
    explicit t(typename t::term_type term, Handler hnd = {}):
        base(hnd), term(term) {}
    //! Creates the rule
    /*! \param[in] term the terminal symbol matched by this rule
     * \param[in] out a place where a matched terminal will be stored */
    explicit t(typename t::term_type term, typename t::term_type& out):
        t(term, [&out](auto&&, auto&&, auto&& it, auto&&) { out = *it; }) {}
protected:
    //! \copydoc rule_base::eval()
    typename t::parse_result eval([[maybe_unused]] Ctx& ctx,
                                  [[maybe_unused]] empty& tmp,
                                  It begin, It end) const
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

namespace impl {

//! A child rule type with removed reference, \c const, \c volatile
/*! \tparam R a rule type */
template <rule_cvref R> using child_t = std::remove_cvref_t<R>;

//! A rule context type
/*! \tparam R a rule type */
template <rule_cvref R> using context_t = typename child_t<R>::context_type;

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
 * \tparam Handler the type of a handler called when the rule matches */
template <rule_cvref Child,
    class Handler = default_handler<impl::context_t<Child>, size_t,
        impl::iterator_t<Child>>>
class repeat final: public rule_base<repeat<Child, Handler>,
    impl::context_t<Child>, size_t, impl::iterator_t<Child>, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<repeat<Child, Handler>, impl::context_t<Child>,
        size_t, impl::iterator_t<Child>, Handler>;
    //! The type of the child rule
    using child_type = Child;
    //! It denotes an unlimited maximum number of occurrences of the child node
    static constexpr size_t unlimited = 0;
    //! Creates the rule with a handler.
    /*! \param[in] child the child node
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches
     * \param[in] hnd the handler stored in the rule
     * \note Template magic ensures that \a Handler is not deduced to an
     * integral type when called without an explicit \a Handler, e.g., from
     * \c operator-(). */
    template <class = void> requires (!std::convertible_to<Handler, size_t>)
    explicit repeat(Child child, size_t min = 0, size_t max = unlimited,
                    Handler hnd = {}):
        base(hnd), _child(std::forward<Child>(child)), min(min), max(max) {}
    //! Creates the rule with a handler.
    /*! \param[in] child the child node
     * \param[in] hnd the handler stored in the rule
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches
     * \note Template magic ensures that \a Handler is not deduced to an
     * integral type when called without an explicit \a Handler. */
    template <class = void> requires (!std::convertible_to<Handler, size_t>)
    repeat(Child child, Handler hnd, size_t min = 0, size_t max = unlimited):
        repeat(std::forward<Child>(child), min, max, hnd) {}
    //! Creates the rule.
    /*! \param[in] child the child rule
     * \param[in] out a place where the number of matches will be stored if the
     * child rule matches at least \a min times
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches
     * maches between \ref min and \ref max times */
    repeat(Child child, size_t& out, size_t min = 0, size_t max = unlimited):
        repeat(std::forward<Child>(child),
               [&out](auto&&, size_t info, auto&&, auto&&) {
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
                   typename repeat::iterator_type begin,
                   typename repeat::iterator_type end) const
    {
        size_t tmp = 0;
        return this->parse_with_tmp(ctx, tmp, begin, end);
    }
    //! \copydoc rule_base::eval()
    typename repeat::parse_result eval(typename repeat::context_type& ctx,
                                       size_t& tmp,
                                       typename repeat::iterator_type begin,
                                       typename repeat::iterator_type end) const
    {
        for (size_t i = 0;; ++i) {
            switch (auto [result, pos] = _child.parse(ctx, begin, end); result)
            {
            case rule_result::fail:
            case rule_result::fail_final:
                if (i >= min) {
                    tmp = i;
                    return {rule_result::ok, pos};
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
/*! \tparam Child a child rule type */
template <class Child>
repeat(Child&&, size_t = {}, size_t = {}) -> repeat<Child>;

//! A deduction guide for \ref repeat
/*! \tparam Child a child rule type
 * \tparam Handler a handler type */
template <class Child, class Handler>
    requires (!std::convertible_to<Handler, size_t>)
repeat(Child&&, size_t, size_t, Handler) -> repeat<Child, Handler>;

//! A deduction guide for \ref repeat
/*! \tparam Child a child rule type
 * \tparam Handler a handler type */
template <class Child, class Handler>
    requires (!std::convertible_to<Handler, size_t>)
repeat(Child&&, Handler, size_t = {}, size_t = {}) -> repeat<Child, Handler>;

//! A deduction guide for \ref repeat
/*! \tparam Child a child rule type */
template <class Child>
repeat(Child&&, size_t&, size_t = {}, size_t = {}) -> repeat<Child>;

//! A rule that matches a sequential composition of two child rules
/*! It matches iff the first child matches an initial part of the input and the
 * second child matches an immediately following part of the input.
 * \tparam Child1 the first child rule type
 * \tparam Child2 the second child rule type
 * \tparam C1 type \a Child1 with removed reference, \c const, \c volatile
 * \tparam C2 type \a Child2 with removed reference, \c const, \c volatile
 * \tparam Handler the type of a handler called when the rule matches */
template <rule_cvref Child1, rule_cvref Child2,
    class Handler = default_handler<impl::context_t<Child1>, empty,
        impl::iterator_t<Child1>>>
    requires
        std::same_as<impl::context_t<Child1>, impl::context_t<Child2>> &&
        std::same_as<impl::iterator_t<Child1>, impl::iterator_t<Child2>> &&
        std::same_as<impl::handler_t<Child1>, impl::handler_t<Child2>>
class seq final: public rule_base<seq<Child1, Child2, Handler>,
    impl::context_t<Child1>, empty, impl::iterator_t<Child1>, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<seq<Child1, Child2, Handler>,
        impl::context_t<Child1>, empty, impl::iterator_t<Child1>, Handler>;
    //! The type of the first child rule
    using child1_type = Child1;
    //! The type of the second child rule
    using child2_type = Child2;
    //! Creates the rule with a handler.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in] hnd the handler stored in the rule */
    seq(Child1 child1, Child2 child2, Handler hnd = {}):
        base(hnd), _child1(std::forward<Child1>(child1)),
        _child2(std::forward<Child2>(child2)) {}
    //! Creates the rule.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in] out a place where \c true will be stored if the sequence of
     * child rules matches */
    seq(Child1 child1, Child2 child2, bool& out):
        seq(std::forward<Child1>(child1), std::forward<Child2>(child2),
            [&out](auto&&, auto&&, auto&&, auto&&) { out = true; }) {}
    //! Gets the first child node.
    /*! \return the child */
    const Child1& child1() const noexcept { return _child1; }
    //! Gets the second child node.
    /*! \return the child */
    const Child2& child2() const noexcept { return _child2; }
protected:
    //! \copydoc rule_base::eval()
    typename seq::parse_result eval(typename seq::context_type& ctx,
                                    [[maybe_unused]] empty& tmp,
                                    typename seq::iterator_type begin,
                                    typename seq::iterator_type end) const
    {
        switch (auto [result1, pos1] = _child1.parse(ctx, begin, end); result1)
        {
        case rule_result::fail:
        case rule_result::fail_final:
            return {result1, pos1};
        case rule_result::ok:
        case rule_result::ok_final:
            switch (auto [result2, pos2] = _child2.parse(ctx, pos1, end);
                    result2)
            {
            case rule_result::fail:
            case rule_result::fail_final:
                if (result1 == rule_result::ok_final)
                    result2 = rule_result::fail_final;
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
 * \tparam Child2 a second child rule type */
template <class Child1, class Child2>
seq(Child1&&, Child2&&) -> seq<Child1, Child2>;

//! A deduction guide for \ref seq
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type
 * \tparam Handler a handler type */
template <class Child1, class Child2, class Handler>
seq(Child1&&, Child2&&, Handler) -> seq<Child1, Child2, Handler>;

//! A deduction guide for \ref seq
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type */
template <class Child1, class Child2>
seq(Child1&&, Child2&&, bool&) -> seq<Child1, Child2>;

//! A rule that matches an alternative of two child rules
/*! It matches iff the first or the second child matches. If the first child
 * matches, matching of the second child is not tried.
 * \tparam Child1 the first child rule type
 * \tparam Child2 the second child rule type
 * \tparam C1 type \a Child1 with removed reference, \c const, \c volatile
 * \tparam C2 type \a Child2 with removed reference, \c const, \c volatile
 * \tparam Handler the type of a handler called when the rule matches */
template <rule_cvref Child1, rule_cvref Child2,
    class Handler = default_handler<typename Child1::context_type, empty,
        typename Child1::iterator_type>>
    requires
        std::same_as<impl::context_t<Child1>, impl::context_t<Child2>> &&
        std::same_as<impl::iterator_t<Child1>, impl::iterator_t<Child2>> &&
        std::same_as<impl::handler_t<Child1>, impl::handler_t<Child2>>
class alt final: public rule_base<alt<Child1, Child2, Handler>,
    impl::context_t<Child1>, empty, impl::iterator_t<Child1>, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<alt<Child1, Child2, Handler>,
        impl::context_t<Child1>, empty, impl::iterator_t<Child1>, Handler>;
    //! The type of the first child rule
    using child1_type = Child1;
    //! The type of the second child rule
    using child2_type = Child2;
    //! Creates the rule with a handler.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in] hnd the handler stored in the rule */
    alt(Child1 child1, Child2 child2, Handler hnd = {}):
        base(hnd), _child1(std::forward<Child1>(child1)),
        _child2(std::forward<Child2>(child2)) {}
    //! Creates the rule.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in] out a place where \c true will be stored if either of the
     * alternative child rules matches */
    alt(Child1 child1, Child2 child2, bool& out):
        alt(std::forward<Child1>(child1), std::forward<Child2>(child2),
            [&out](auto&&, auto&&, auto&&, auto&&) { out = true; }) {}
    //! Gets the first child node.
    /*! \return the child */
    const Child1& child1() const noexcept { return _child1; }
    //! Gets the second child node.
    /*! \return the child */
    const Child2& child2() const noexcept { return _child2; }
protected:
    //! \copydoc rule_base::eval()
    typename alt::parse_result eval(typename alt::context_type& ctx,
                                    [[maybe_unused]] empty& tmp,
                                    typename alt::iterator_type begin,
                                    typename alt::iterator_type end) const
    {
        switch (auto [result1, pos1] = _child1.parse(ctx, begin, end); result1)
        {
        case rule_result::fail_final:
            return {rule_result::fail_final};
        case rule_result::fail:
            return _child2.parse(ctx, pos1, end);
        case rule_result::ok:
        case rule_result::ok_final:
            return {result1, pos1};
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
 * \tparam Handler a handler type */
template <class Child1, class Child2, class Handler>
alt(Child1&&, Child2&&, Handler) -> alt<Child1, Child2, Handler>;

//! A deduction guide for \ref alt
/*! \tparam Child1 a first child rule type
 * \tparam Child2 a second child rule type */
template <class Child1, class Child2>
alt(Child1&&, Child2&&, bool&) -> alt<Child1, Child2>;

//! A rule that disables (cuts) the following alternatives in rules::alt
/*! It this node is a member of a sequence of rule::seq nodes that is a child
 * of a rule::alt node, than after this node is evaluated (regardless if it
 * matches or not), the following alternatives in the sequence of rules::alt
 * nodes will not be matched.
 * \tparam Child a child rule type
 * \tparam C type \a Child with removed reference, \c const, \c volatile */
template <rule_cvref Child>
class cut final:
    public rule_base<cut<Child>, typename Child::context_type, empty,
        typename Child::iterator_type, typename Child::handler_type>
{
public:
    //! The alias for the base class
    using base = rule_base<cut<Child>, impl::context_t<Child>,
        impl::iterator_t<Child>, impl::handler_t<Child>>;
    //! The type of the child rule
    using child_type = Child;
    //! Creates the rule
    /*! \param[in] child the child node */
    explicit cut(Child child): base(), _child(std::forward<Child>(child)) {}
    //! Gets the child node.
    /*! \return the child */
    const Child& child() const noexcept { return _child; }
protected:
    //! \copydoc rule_base::eval()
    typename cut::parse_result eval(typename cut::context_type& ctx,
                                    [[maybe_unused]] empty& tmp,
                                    typename cut::iterator_type begin,
                                    typename cut::iterator_type end) const
    {
        switch (auto [result, pos] = _child.parse(ctx, begin, end); result) {
        case rule_result::fail:
        case rule_result::fail_final:
            return {rule_result::fail_final, pos};
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
/*! \tparam Child a child rule type */
template <class Child>
cut(Child&&) -> cut<Child>;

} // namespace rules

//! Wraps a rule by rules::repeat for 0 or 1 match.
/*! \tparam Rule a type of the child rule
 * \param[in] r a child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 0 and maximum 1 */
template <rule_cvref Rule>
rules::repeat<Rule, rebind_rhnd_t<Rule, size_t>> operator-(Rule&& r)
{
    return rules::repeat{std::forward<Rule>(r), 0, 1};
}

//! Wraps a rule by rules::repeat for at least 1 match.
/*! \tparam Rule a type of the child rule
 * \param[in] r a child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 1 and maximum unlimited */
template <rule_cvref Rule>
rules::repeat<Rule, rebind_rhnd_t<Rule, size_t>> operator+(Rule&& r)
{
    return rules::repeat{std::forward<Rule>(r), 1,
        rules::repeat<Rule>::unlimited};
}

//! Wraps a rule by rules::repeat for any number of matches.
/*! \tparam Rule the type of the child rule
 * \param[in] r the child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 0 and maximum unlimited */
template <rule_cvref Rule>
rules::repeat<Rule, rebind_rhnd_t<Rule, size_t>> operator*(Rule&& r)
{
    return rules::repeat{std::forward<Rule>(r), 0,
        rules::repeat<Rule>::unlimited};
}

//! Creates a sequential composition of two rules.
/*! \tparam Rule1 the type of the first child rule
 * \tparam Rule2 the type of the second child rule
 * \param[in] r1 the first child rule
 * \param[in] r2 the second child rule
 * \return a rules::seq rule containing \a r1 and \a r2 as child rules */
template <rule Rule1, rule_cvref Rule2>
rules::seq<Rule1, Rule2, rebind_rhnd_t<Rule1, empty>>
operator<<(Rule1&& r1, Rule2&& r2)
{
    return rules::seq{std::forward<Rule1>(r1), std::forward<Rule2>(r2)};
}

//! Creates an alternative composition of two rules.
/*! \tparam Rule1 the type of the first child rule
 * \tparam Rule2 the type of the second child rule
 * \param[in] r1 the first child rule
 * \param[in] r2 the second child rule
 * \return a rules::alt rule containing \a r1 and \a r2 as child rules */
template <rule_cvref Rule1, rule_cvref Rule2>
rules::alt<Rule1, Rule2, rebind_rhnd_t<Rule1, empty>>
operator|(Rule1 r1, Rule2 r2)
{
    return rules::alt{std::move(r1), std::move(r2)};
}

//! Creates a rule that disables (cuts) the following alternatives in rules::alt
/*! \tparam Rule the type of the child rule
 * \param[in] r the child rule
 * \return a rules::cut rule containing \a r as the child rule. */
template <rule_cvref Rule, rebind_rhnd_t<Rule, empty>>
rules::cut<Rule> operator!(Rule r)
{
    return rules::cut{std::move(r)};
}

} // namespace threadscript::parser
