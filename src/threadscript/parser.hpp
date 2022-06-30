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
/*! \tparam Handler a handler
 * \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols */
template <class Handler, class Ctx, class It> concept handler =
    std::is_invocable_r_v<void, Handler, Ctx&, It, It>;

//! The default handler called when a rule matches
/*! \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \param[in] ctx a parsing context
 * \param[in] begin the first terminal matching a rule
 * \param[in] end one after the last terminal matching a rule */
template <class Ctx, std::forward_iterator It>
using default_handler = std::function<void(Ctx& ctx, It begin, It end)>;

//! A base class of all rules
/*! \tparam Rule the derived rule class
 * \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches
 * \threadsafe{safe,unsafe} */
template <class Rule, class Ctx, std::forward_iterator It,
    handler<Ctx, It> Handler = default_handler<Ctx, It>>
class rule_base {
public:
    using rule_type = Rule; //!< The derived rule type
    using context_type = Ctx; //!< A parsing context
    using iterator_type = It; //!< An iterator to the input
    //! The type of a terminal symbol
    using term_type = std::remove_cvref_t<decltype(*std::declval<It>())>;
    //! The type of a handler called when the rule matches
    using handler_type = Handler;
    //! The result type of parse() and eval()
    using parse_result = std::pair<rule_result, It>;
    //! Creates the rule with a handler
    /*! \param[in] hnd the handler stored in the rule */
    explicit rule_base(Handler hnd = {}): hnd(std::move(hnd)) {}
    //! Parses a part of the input according to the rule and uses the result.
    /*! This function consists of two parts: parsing the input in function
     * eval() and using the matching part of input, usually by passing it to a
     * \a Handler, in function attr(), called only if the rule matches. Both
     * function are nonvirtual, but may be overriden in a derived class.
     * Overriding implementations are resolved statically using the type
     * information in template parameter \a Rule. If the rule matches, it
     * returns rule_result::ok or rule_result::ok_final and the iterator
     * denoting the end of the matched part of input. If the rule does not
     * match, it returns rule_result::fail or rule_result::fail_final and the
     * position where mathing failed.
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \return[in] the result of matching this rule with the input sequence
     * \throw error if the mximum depth of rule nesting is exceeded */
    parse_result parse(Ctx& ctx, It begin, It end) const {
        if (ctx.max_depth && ctx.depth >= ctx.max_depth)
            throw error(begin, ctx.depth_msg);
        finally dec_depth{[&d = ctx.depth]() noexcept { --d; }};
        ++ctx.depth;
        auto result = static_cast<const Rule*>(this)->eval(ctx, begin, end);
        if (result.first == rule_result::ok ||
            result.first == rule_result::ok_final)
        {
            static_cast<const Rule*>(this)->attr(ctx, begin, result.second);
        }
        return result;
    }
    //! Parses a part of the input according to the rule.
    /*! This function decides if the input matches the rule. The default
     * implementation does not consume any input and fails (with
     * rule_result::fail).
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \return[in] the result of matching this rule with the input sequence,
     * with the same meaning as in parse() */
    parse_result eval([[maybe_unused]] Ctx& ctx, [[maybe_unused]] It begin,
                      [[maybe_unused]] It end) const
    {
        return {rule_result::fail, begin};
    }
    //! Handles the matching part of the input.
    /*! The default implementation calls the handler stored in the rule object.
     * If the handler is convertible to \c bool, as default_handler is, it is
     * called only if it evaluates to \c true.
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in] begin a position in the input sequence where matching starts;
     * it is the \a begin argument of parse()
     * \param[in] end the end of matching part of the input, as returned by
     * eval()
     * \note It is named \c attr, because its typical functionality is stored
     * a value obtained from the matching part of input into the attribute of
     * an attribute grammar. */
    void attr(Ctx& ctx, It begin, It end) const {
        if constexpr (requires { bool(hnd); }) {
            if (bool(hnd))
                hnd(ctx, begin, end);
        } else
            hnd(ctx, begin, end);
    }
    //! Sets a new handler
    /*! \param[in] h the new handler
     * \return \c *this */
    Rule& operator[](Handler h) {
        hnd = std::move(h);
        return static_cast<Rule&>(*this);
    }
    //! The stored handler called by attr().
    Handler hnd{};
};

//! A rule, which must be derived from rule_base.
template <class Rule> concept rule = std::derived_from<Rule, rule_base<
    Rule, typename Rule::context_type, typename Rule::iterator_type,
    typename Rule::handler_type>> &&
    requires (const Rule r, typename Rule::context_type& ctx,
              const typename Rule::iterator_type it)
    {
        typename Rule::parse_result;
        { r.parse(ctx, it, it) } -> std::same_as<typename Rule::parse_result>;
        { r.eval(ctx, it, it) } -> std::same_as<typename Rule::parse_result>;
        { r.attr(ctx, it, it) } -> std::same_as<void>;
    };

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
    template <class Rule, class Ctx, std::forward_iterator It,
        handler<Ctx, It> Handler> friend class rule_base;
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
/*! \test in file test_parser.cpp */
namespace rules {

//! A rule that always fails.
/*! \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, It>>
class fail: public rule_base<fail<Ctx, It, Handler>, Ctx, It, Handler> {
    using rule_base<fail, Ctx, It, Handler>::rule_base;
};

//! A rule that matches the end of input.
/*! This rule does not consume any input, therefore it must not be used in an
 * unlimited rules::repeat, because it would create an endless loop.
 * \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, It>>
class eof: public rule_base<eof<Ctx, It, Handler>, Ctx, It, Handler> {
    using rule_base<eof, Ctx, It, Handler>::rule_base;
public:
    //! \copydoc rule_base::eval()
    typename eof::parse_result eval([[maybe_unused]] Ctx& ctx, It begin,
                                    It end) const
    {
        return {begin == end ? rule_result::ok : rule_result::fail, begin};
    }
};

//! A rule that matches any single terminal symbol.
/*! \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, It>>
class any: public rule_base<any<Ctx, It, Handler>, Ctx, It, Handler> {
    using rule_base<any, Ctx, It, Handler>::rule_base;
public:
    //! Creates the rule
    /*! \param[in] out a place where a matched terminal will be stored */
    explicit any(typename any::term_type& out):
        any([&out](auto&&, auto&& it, auto&&) { out = *it; }) {}
    //! \copydoc rule_base::eval()
    typename any::parse_result eval([[maybe_unused]] Ctx& ctx, It begin,
                                    It end) const
    {
        if (begin != end)
            return {rule_result::ok, ++begin};
        else
            return {rule_result::fail, begin};
    }
};

//! A rule that matches a specific single terminal symbol.
/*! \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, It>>
class t: public rule_base<t<Ctx, It, Handler>, Ctx, It, Handler> {
public:
    //! The alias for the base class
    using base = rule_base<t<Ctx, It, Handler>, Ctx, It, Handler>;
    //! Creates the rule with a handler.
    /*! \param[in] term the terminal symbol matched by this rule
     * \param[in] hnd the handler stored in the rule */
    explicit t(typename t::term_type term, Handler hnd = {}):
        base(hnd), term(term) {}
    //! Creates the rule
    /*! \param[in] term the terminal symbol matched by this rule
     * \param[in] out a place where a matched terminal will be stored */
    explicit t(typename t::term_type term, typename t::term_type& out):
        t(term, [&out](auto&&, auto&& it, auto&&) { out = *it; }) {}
    //! \copydoc rule_base::eval()
    typename t::parse_result eval([[maybe_unused]] Ctx& ctx, It begin,
                                  It end) const
    {
        if (begin != end && *begin == term)
                return {rule_result::ok, ++begin};
        else
            return {rule_result::fail, begin};
    }
private:
    typename t::term_type term; //!< The terminal symbol matched by this rule
};

//! A rule that matches some number of occurrences of a child rule 
/*! \tparam Child a child rule type
 * \tparam Handler the type of a handler called when the rule matches */
template <rule Child,
    class Handler = default_handler<typename Child::context_type,
        typename Child::iterator_type>>
class repeat: public rule_base<repeat<Child, Handler>,
    typename Child::context_type, typename Child::iterator_type, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<repeat<Child, Handler>,
        typename Child::context_type, typename Child::iterator_type, Handler>;
    //! The type of the child rule
    using child_type = Child;
    //! It denotes an unlimited maximum number of occurrences of the child node
    static constexpr size_t unlimited = 0;
    //! Creates the rule with a handler.
    /*! \param[in] child the child node
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches
     * \param[in] hnd the handler stored in the rule */
    explicit repeat(Child child, size_t min = 0, size_t max = unlimited,
                    Handler hnd = {}):
        base(hnd), child(std::move(child)), min(min), max(max) {}
    //! Creates the rule with a handler.
    /*! \param[in] child the child node
     * \param[in] hnd the handler stored in the rule
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches */
    repeat(Child child, Handler hnd, size_t min = 0, size_t max = unlimited):
        repeat(child, min, max, hnd) {}
    //! Creates the rule.
    /*! \param[in] child the child rule
     * \param[in] out a place where \c true will be stored if the child rule
     * \param[in] min the minimum number of child matches
     * \param[in] max the maximum number of child matches
     * maches between \ref min and \ref max times */
    repeat(Child child, bool& out, size_t min = 0, size_t max = unlimited):
        repeat(std::move(child), [&out](auto&&, auto&&, auto&&) { out = true; },
               min, max) {}
    //! \copydoc rule_base::eval()
    typename repeat::parse_result eval(typename repeat::context_type& ctx,
                                       typename repeat::iterator_type begin,
                                       typename repeat::iterator_type end) const
    {
        for (size_t i = 0;; ++i) {
            switch (auto [result, pos] = child.parse(ctx, begin, end); result) {
            case rule_result::fail:
            case rule_result::fail_final:
                if (i >= min)
                    return {rule_result::ok, pos};
                else
                    return {rule_result::fail, pos};
            case rule_result::ok:
            case rule_result::ok_final:
                if (max == unlimited || i < max - 1)
                    break;
                else
                    return {rule_result::ok, pos};
            default:
                assert(false);
            }
        }
    }
private:
    Child child; //!< The child node
    size_t min = 0; //!< The minimum number of matches of the child node
    size_t max = unlimited; //!< The maximum number of matches of the child node
};

//! A rule that matches a sequential composition of two child rules
/*! It matches iff the first child matches an initial part of the input and the
 * second child matches an immediately following part of the input.
 * \tparam Child1 the first child rule type
 * \tparam Child2 the second child rule type
 * \tparam Handler the type of a handler called when the rule matches */
template <rule Child1, rule Child2,
    class Handler = default_handler<typename Child1::context_type,
        typename Child1::iterator_type>>
    requires
        std::same_as<typename Child1::context_type,
            typename Child2::context_type> &&
        std::same_as<typename Child1::iterator_type,
            typename Child2::iterator_type> &&
        std::same_as<typename Child1::handler_type,
            typename Child2::handler_type>
class seq: public rule_base<seq<Child1, Child2, Handler>,
    typename Child1::context_type, typename Child1::iterator_type, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<seq<Child1, Child2, Handler>,
        typename Child1::context_type, typename Child1::iterator_type, Handler>;
    //! The type of the first child rule
    using child1_type = Child1;
    //! The type of the second child rule
    using child2_type = Child2;
    //! Creates the rule with a handler.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in] hnd the handler stored in the rule */
    seq(Child1 child1, Child2 child2, Handler hnd = {}):
        base(hnd), child1(std::move(child1)), child2(std::move(child2)) {}
    //! Creates the rule.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in] out a place where \c true will be stored if the sequence of
     * child rules matches */
    seq(Child1 child1, Child2 child2, bool& out):
        seq(std::move(child1), std::move(child2),
            [&out](auto&&, auto&&, auto&&) { out = true; }) {}
    //! \copydoc rule_base::eval()
    typename seq::parse_result eval(typename seq::context_type& ctx,
                                    typename seq::iterator_type begin,
                                    typename seq::iterator_type end) const
    {
        switch (auto [result1, pos1] = child1.parse(ctx, begin, end); result1) {
        case rule_result::fail:
        case rule_result::fail_final:
            return {result1, pos1};
        case rule_result::ok:
        case rule_result::ok_final:
            switch (auto [result2, pos2] = child2.parse(ctx, pos1, end);
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
    Child1 child1; //!< The first child node
    Child2 child2; //!< The second child node
};

//! A rule that matches an alternative of two child rules
/*! It matches iff the first or the second child matches. If the first child
 * matches, matching of the second child is not tried.
 * \tparam Child1 the first child rule type
 * \tparam Child2 the second child rule type
 * \tparam Handler the type of a handler called when the rule matches */
template <rule Child1, rule Child2,
    class Handler = default_handler<typename Child1::context_type,
        typename Child1::iterator_type>>
    requires
        std::same_as<typename Child1::context_type,
            typename Child2::context_type> &&
        std::same_as<typename Child1::iterator_type,
            typename Child2::iterator_type> &&
        std::same_as<typename Child1::handler_type,
            typename Child2::handler_type>
class alt: public rule_base<alt<Child1, Child2, Handler>,
    typename Child1::context_type, typename Child1::iterator_type, Handler>
{
public:
    //! The alias for the base class
    using base = rule_base<alt<Child1, Child2, Handler>,
        typename Child1::context_type, typename Child1::iterator_type, Handler>;
    //! The type of the first child rule
    using child1_type = Child1;
    //! The type of the second child rule
    using child2_type = Child2;
    //! Creates the rule with a handler.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in] hnd the handler stored in the rule */
    alt(Child1 child1, Child2 child2, Handler hnd = {}):
        base(hnd), child1(std::move(child1)), child2(std::move(child2)) {}
    //! Creates the rule.
    /*! \param[in] child1 the first child node
     * \param[in] child2 the second child node
     * \param[in] out a place where \c true will be stored if either of the
     * alternative child rules matches */
    alt(Child1 child1, Child2 child2, bool& out):
        alt(std::move(child1), std::move(child2),
            [&out](auto&&, auto&&, auto&&) { out = true; }) {}
    //! \copydoc rule_base::eval()
    typename alt::parse_result eval(typename alt::context_type& ctx,
                                    typename alt::iterator_type begin,
                                    typename alt::iterator_type end) const
    {
        switch (auto [result1, pos1] = child1.parse(ctx, begin, end); result1) {
        case rule_result::fail_final:
            return {rule_result::fail_final};
        case rule_result::fail:
            return child2.parse(ctx, pos1, end);
        case rule_result::ok:
        case rule_result::ok_final:
            return {result1, pos1};
        default:
            assert(false);
        }
    }
private:
    Child1 child1; //!< The first child node
    Child2 child2; //!< The second child node
};

//! A rule that disables (cuts) the following alternatives in rules::alt
/*! It this node is a member of a sequence of rule::seq nodes that is a child
 * of a rule::alt node, than after this node is evaluated (regardless if it
 * matches or not), the following alternatives in the sequence of rules::alt
 * nodes will not be matched.
 * \tparam Child a child rule type */
template <rule Child>
class cut: public rule_base<cut<Child>, typename Child::context_type,
    typename Child::iterator_type, typename Child::handler_type>
{
public:
    //! The alias for the base class
    using base = rule_base<cut<Child>, typename Child::context_type,
        typename Child::iterator_type, typename Child::handler_type>;
    //! The type of the child rule
    using child_type = Child;
    //! Creates the rule
    /*! \param[in] child the child node */
    explicit cut(Child child): base(), child(std::move(child)) {}
    //! \copydoc rule_base::eval()
    typename cut::parse_result eval(typename cut::context_type& ctx,
                                    typename cut::iterator_type begin,
                                    typename cut::iterator_type end) const
    {
        switch (auto [result, pos] = child.parse(ctx, begin, end); result) {
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
    Child child; //!< The child node
};

} // namespace rules

//! Wraps a rule by rules::repeat for 0 or 1 match.
/*! \tparam Rule a type of the child rule
 * \param[in] r a child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 0 and maximum 1 */
template <rule Rule> rules::repeat<Rule> operator-(Rule r)
{
    return rules::repeat<Rule>(std::move(r), 0, 1);
}

//! Wraps a rule by rules::repeat for at least 1 match.
/*! \tparam Rule a type of the child rule
 * \param[in] r a child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 1 and maximum unlimited */
template <rule Rule>
rules::repeat<Rule, typename Rule::handler_type> operator+(Rule r)
{
    return {std::move(r), 1, rules::repeat<Rule>::unlimited};
}

//! Wraps a rule by rules::repeat for any number of matches.
/*! \tparam Rule the type of the child rule
 * \param[in] r the child rule
 * \return a rules::repeat rule containg \a r as the child rule, with the
 * minimum number of repetitions 0 and maximum unlimited */
template <rule Rule>
rules::repeat<Rule, typename Rule::handler_type> operator*(Rule r)
{
    return {std::move(r), 0, rules::repeat<Rule>::unlimited};
}

//! Creates a sequential composition of two rules.
/*! \tparam Rule1 the type of the first child rule
 * \tparam Rule2 the type of the second child rule
 * \param[in] r1 the first child rule
 * \param[in] r2 the second child rule
 * \return a rules::seq rule containing \a r1 and \a r2 as child rules */
template <rule Rule1, rule Rule2>
rules::seq<Rule1, Rule2, typename Rule1::handler_type> operator<<(Rule1 r1,
                                                                  Rule2 r2)
{
    return {std::move(r1), std::move(r2)};
}

//! Creates an alternative composition of two rules.
/*! \tparam Rule1 the type of the first child rule
 * \tparam Rule2 the type of the second child rule
 * \param[in] r1 the first child rule
 * \param[in] r2 the second child rule
 * \return a rules::alt rule containing \a r1 and \a r2 as child rules */
template <rule Rule1, rule Rule2>
rules::alt<Rule1, Rule2, typename Rule1::handler_type> operator|(Rule1 r1,
                                                                 Rule2 r2)
{
    return {std::move(r1), std::move(r2)};
}

//! Creates a rule that disables (cuts) the following alternatives in rules::alt
/*! \tparam Rule the type of the child rule
 * \param[in] r the child rule
 * \return a rules::cut rule containing \a r as the child rule. */
template <rule Rule> rules::cut<Rule> operator!(Rule r)
{
    return std::move(r);
}

} // namespace threadscript::parser
