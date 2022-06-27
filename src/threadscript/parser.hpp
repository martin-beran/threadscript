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

template <class Ctx, std::forward_iterator It>
using default_handler = std::function<rule_result(Ctx&, const It&, const It&)>;

template <class Rule, class Ctx, std::forward_iterator It,
    handler<Ctx, It> Handler = default_handler<Ctx, It>>
class rule_base {
public:
    using context_type = Ctx;
    using iterator_type = It;
    using term_type = std::remove_cvref_t<decltype(*std::declval<It>())>;
    using handler_type = Handler;
    explicit rule_base(Handler hnd = {}): hnd(std::move(hnd)) {}
    rule_result parse(Ctx& ctx, It& pos, const It& end) const {
        if (ctx.max_depth && ctx.depth >= ctx.max_depth)
            throw error(pos, ctx.depth_msg);
        It begin = pos;
        finally dec_depth{[&d = ctx.depth]() noexcept { --d; }};
        ++ctx.depth;
        if (auto result = static_cast<const Rule*>(this)->eval(ctx, pos, end);
            result == rule_result::fail)
        {
            return result;
        } else
            switch (static_cast<const Rule*>(this)->attr(ctx, begin, pos)) {
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
                     [[maybe_unused]] const It& end) const
    {
        return rule_result::fail;
    }
    rule_result attr(Ctx& ctx, const It& begin, const It& end) const {
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
    Rule, typename Rule::context_type, typename Rule::iterator_type,
    typename Rule::handler_type>>;

class context {
public:
    template <rule R> void parse(const R& rule,
                                 typename R::iterator_type& pos,
                                 const typename R::iterator_type& end)
    {
        depth = 0;
        switch (rule.parse(*this, pos, end)) {
            case rule_result::fail:
                if (error_msg)
                    throw error(pos, *error_msg);
                else
                    throw error(pos);
            case rule_result::ok:
            case rule_result::ok_final:
                break;
            default:
                assert(false);
        }
    }
    template <rule R>
    void parse(const R& rule,
            std::pair<typename R::iterator_type, typename R::iterator_type>& it)
    {
        parse(rule, it.first, it.second);
    }
    std::optional<std::string> error_msg{};
    std::optional<size_t> max_depth{};
    size_t depth = 0;
    std::string depth_msg{"Maximum parsing depth exceeded"};
};

template <std::forward_iterator It> requires
    requires (It it) { { *it } -> std::same_as<char&>; } ||
    requires (It it) { { *it } -> std::same_as<const char&>; }
class script_iterator: public std::forward_iterator_tag {
public:
    using difference_type = typename It::difference_type;
    using value_type = typename It::value_type;
    using pointer = typename It::pointer;
    using reference = typename It::reference;
    using iterator_category = typename std::forward_iterator_tag;
    script_iterator() = default;
    explicit script_iterator(It it, size_t line = 1, size_t column = 1):
        line(line), column(column), it(std::move(it)) {}
    It get() const { return it; }
    auto& operator*() const { return *it; }
    bool operator==(const script_iterator& o) const {
        return it == o.it;
    }
    script_iterator& operator++() {
        step();
        ++it;
        return *this;
    }
    script_iterator operator++(int) {
        auto result = *this;
        step();
        ++it;
        return *this;
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

template <class T>
std::pair<script_iterator<typename T::const_iterator>,
    script_iterator<typename T::const_iterator>>
make_script_iterator(const T& chars)
{
    return {script_iterator(chars.cbegin()), script_iterator(chars.cend())};
}

// This namespace contains various reusable parser rules
namespace rules {

template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, It>>
class fail: public rule_base<fail<Ctx, It, Handler>, Ctx, It, Handler> {
    using rule_base<fail, Ctx, It, Handler>::rule_base;
};

template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, It>>
class eof: public rule_base<eof<Ctx, It, Handler>, Ctx, It, Handler> {
    using rule_base<eof, Ctx, It, Handler>::rule_base;
public:
    rule_result eval([[maybe_unused]] Ctx& ctx, It& begin, const It& end) const
    {
        return begin == end ? rule_result::ok : rule_result::fail;
    }
};

template <class Ctx, std::forward_iterator It,
    class Handler = default_handler<Ctx, It>>
class any: public rule_base<any<Ctx, It, Handler>, Ctx, It, Handler> {
    using rule_base<any, Ctx, It, Handler>::rule_base;
public:
    explicit any(typename any::term_type& out):
        any([&out](auto&&, auto&& it, auto&&) {
              out = *it;
              return rule_result::ok;
          }) {}
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
