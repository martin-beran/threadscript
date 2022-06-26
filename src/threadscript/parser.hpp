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

enum class rule_result: uint8_t {
    fail,
    ok,
    ok_final,
};

template <class Handler, class Ctx, class It> concept handler =
    std::is_invocable_r_v<rule_result, Handler, Ctx&, const It&, const It&>;

template <class Ctx, std::forward_iterator It, handler<Ctx, It> F>
class rule_base;

template <class Ctx, std::forward_iterator It,
    handler<Ctx, It> Handler =
        std::function<rule_result(Ctx&, const It&, const It&)>>
class rule_base {
public:
    using context_type = Ctx;
    using iterator_type = It;
    using term_type = decltype(*std::declval<It>());
    using handler_type = Handler;
    explicit rule_base(Handler hnd = {}): hnd(std::move(hnd)) {}
    rule_result parse(Ctx& ctx, It& pos, const It& end) {
        if (ctx.max_depth && ctx.depth >= ctx.max_depth)
            throw error(pos, ctx.depth_msg);
        It begin = pos;
        finally dec_depth{[&d = ctx.depth]() { --d; }};
        ++ctx.depth;
        if (auto result = eval(ctx, pos, end); result == rule_result::fail)
            return result;
        else
            switch (attr(ctx, begin, pos)) {
            case rule_result::fail:
            case rule_result::ok:
                return result;
            case rule_result::ok_final:
                return rule_result::ok_final;
            default:
                assert(false);
            }
    }
    rule_result eval([[maybe_unused]] Ctx& ctx,
                     [[maybe_unused]] It& begin,
                     [[maybe_unused]] const It& end)
    {
        return rule_result::fail;
    }
    rule_result attr(Ctx& ctx, const It& begin, const It& end) {
        if constexpr (requires { bool(hnd); }) {
            if (bool(hnd))
                return hnd(ctx, begin, end);
            else
                return rule_result::ok;
        } else
            return hnd(ctx, begin, end);
    }
private:
    Handler hnd{};
};

template <class Rule> concept rule = std::derived_from<Rule, rule_base<
    typename Rule::context_type, typename Rule::iterator_type,
    typename Rule::handler_type>>;

template <rule Rule> class context {
public:
    using rule_type = Rule;
    using iterator_type = typename Rule::iterator_type;
    using term_type = typename Rule::term_type;
    void parse(const Rule& rule, iterator_type& pos, const iterator_type& end) {
        depth = 0;
        switch (rule.parse(*this, pos, end)) {
            case rule_result::fail:
                if (error_msg)
                    throw error(pos, error_msg);
                else
                    throw error(pos);
            case rule_result::ok:
            case rule_result::ok_final:
                break;
            default:
                assert(false);
        }
    }
    std::optional<std::string> error_msg{};
    std::optional<size_t> max_depth{};
    size_t depth = 0;
    std::string depth_msg{"Maximum parsing depth exceeded"};
};

template <std::forward_iterator It>
requires requires (It it) { { *it } -> std::same_as<char>; }
class script_iterator {
public:
    explicit script_iterator(It it, size_t line = 1, size_t column = 1):
        line(line), column(column), it(std::move(it)) {}
    It get() const { return it; }
    char operator*() const { return *it; }
    script_iterator& operator++() {
        step();
        ++it;
        return *this;
    }
    script_iterator operator++(int) {
        step();
        return script_iterator{it++};
    }
    size_t line = 1;
    size_t column = 1;
private:
    void step() {
        if (*it == '\n') {
            ++line;
            column = 1;
        } else
            ++column;
    }
    It it;
};

// This namespace contains various reusable parser rules
namespace rules {

template <class Ctx, class It, class Handler> class fail:
    rule_base<Ctx, It, Handler>
{
    using rule_base<Ctx, It, Handler>::rule_base;
};

} // namespace rules

} // namespace threadscript::parser
