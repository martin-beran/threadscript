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
    std::is_invocable_r_v<rule_result, Handler, Ctx&, const It&, const It&>;

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
    using context_type = Ctx; //!< A parsing context
    using iterator_type = It; //!< An iterator to the input
    //! The type of a terminal symbol
    using term_type = std::remove_cvref_t<decltype(*std::declval<It>())>;
    //! The type of a handler called when the rule matches
    using handler_type = Handler;
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
    std::pair<rule_result, It> parse(Ctx& ctx, It begin, It end) const {
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
protected:
    //! Parses a part of the input according to the rule.
    /*! This function decides if the input matches the rule. The default
     * implementation does not consume any input and fails (with
     * rule_result::fail).
     * \param[in,out] ctx the parsing context; may be modified by the function
     * \param[in] begin a position in the input sequence where matching starts
     * \param[in] end the end of input sequence
     * \return[in] the result of matching this rule with the input sequence,
     * with the same meaning as in parse() */
    std::pair<rule_result, It> eval([[maybe_unused]] Ctx& ctx,
                                    [[maybe_unused]] It begin,
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
private:
    Handler hnd{}; //!< The stored handler called by attr().
    //! Needed to manipulate context::depth
    friend context;
};

//! A rule, which must be derived from rule_base.
template <class Rule> concept rule = std::derived_from<Rule, rule_base<
    Rule, typename Rule::context_type, typename Rule::iterator_type,
    typename Rule::handler_type>>;

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
/*! \tparam Ctx a parsing context
 * \tparam It an iterator to the input sequence of terminal symbols
 * \tparam Handler the type of a handler called when the rule matches */
template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, It>>
class eof: public rule_base<eof<Ctx, It, Handler>, Ctx, It, Handler> {
    using rule_base<eof, Ctx, It, Handler>::rule_base;
public:
    //! \copydoc rule_base::eval()
    rule_result eval([[maybe_unused]] Ctx& ctx, It& begin, const It& end) const
    {
        return begin == end ? rule_result::ok : rule_result::fail;
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
        any([&out](auto&&, auto&& it, auto&&) {
              out = *it;
              return rule_result::ok;
          }) {}
    //! \copydoc rule_base::eval()
    rule_result eval([[maybe_unused]] Ctx& ctx, It& begin, const It& end) const
    {
        if (begin != end) {
            ++begin;
            return rule_result::ok;
        } else
            return rule_result::fail;
    }
};

} // namespace rules

} // namespace threadscript::parser
