#pragma once

/*! \file
 * \brief The implementation part of predef.hpp
 */

#include "threadscript/predef.hpp"

#include <limits>
#include <syncstream>

namespace threadscript {

namespace predef {

/*** f_add *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_add<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto a1 = this->arg(thread, l_vars, node, narg - 2);
    auto a2 = this->arg(thread, l_vars, node, narg - 1);
    if (!a1 || !a2)
        throw exception::value_null();
    if (auto v1 = dynamic_cast<basic_value_int<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_int<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        // Here, and in other arithmetic operations, we must make sure that if
        // config::value_unsigned_type is smaller than int, we do not perform
        // signed instead of unsigned computation due to integral promotion
        auto s1 = v1->cvalue();
        auto s2 = v2->cvalue();
        config::value_int_type result = uintmax_t(s1) + uintmax_t(s2);
        if ((s1 > 0 && s2 > 0 && (result < s1 || result < s2)) ||
            (s1 < 0 && s2 < 0 && (result > s1 || result > s2)))
        {
            throw exception::op_overflow();
        }
        return this->template make_result<basic_value_int<A>>(thread, l_vars,
                                            node, std::move(result), narg == 3);
    } else if (auto v1 = dynamic_cast<basic_value_unsigned<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_unsigned<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        config::value_unsigned_type result =
            uintmax_t(v1->cvalue()) + uintmax_t(v2->cvalue());
        return this->template make_result<basic_value_unsigned<A>>(thread,
                                    l_vars, node, std::move(result), narg == 3);
    } else if (auto v1 = dynamic_cast<basic_value_string<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_string<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        auto result = v1->cvalue() + v2->cvalue();
        return this->template make_result<basic_value_string<A>>(thread, l_vars,
                                            node, std::move(result), narg == 3);
    } else
        throw exception::value_type();
}

/*** f_and_base **************************************************************/

template <impl::allocator A> bool
f_and_base<A>::eval_impl(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                         const basic_code_node<A>& node, size_t begin)
{
    for (size_t i = begin; i < this->narg(node); ++i)
        if (!f_bool<A>::convert(this->arg(thread, l_vars, node, i)))
            return false;
    return true;
}

/*** f_and *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_and<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = this->eval_impl(thread, l_vars, node);
    return pr;
}

/*** f_and_r *****************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_and_r<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                 const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg == 0)
        throw exception::op_narg();
    bool result = this->eval_impl(thread, l_vars, node, 1);
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                                       std::move(result), true);
}

/*** f_at ********************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_at<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto container = this->arg(thread, l_vars, node, 0);
    auto idx = this->arg(thread, l_vars, node, 1);
    if (!container || !idx)
        throw exception::value_null();
    if (auto pv = dynamic_cast<basic_value_vector<A>*>(container.get())) {
        size_t i = 0;
        if (auto pi = dynamic_cast<basic_value_int<A>*>(idx.get())) {
            if (pi->cvalue() < 0)
                throw exception::value_out_of_range();
            i = size_t(pi->cvalue());
        } else if (auto pi = dynamic_cast<basic_value_unsigned<A>*>(idx.get()))
            i = size_t(pi->cvalue());
        else
            throw exception::value_type();
        if (narg == 2 && i >= pv->cvalue().size())
            throw exception::value_out_of_range();
        if (narg == 2)
            return pv->cvalue()[i];
        else {
            assert(narg == 3);
            if (i >= pv->cvalue().max_size())
                throw exception::value_out_of_range();
            if (i >= pv->cvalue().size())
                pv->value().resize(i + 1);
            return pv->value()[i] = this->arg(thread, l_vars, node, 2);
        }
    } else if (auto ph = dynamic_cast<basic_value_hash<A>*>(container.get())) {
        if (auto pk = dynamic_cast<basic_value_string<A>*>(idx.get())) {
            if (narg == 2) {
                if (auto it = ph->cvalue().find(pk->cvalue());
                    it != ph->cvalue().end())
                {
                    return it->second;
                } else
                    throw exception::value_out_of_range();
            } else {
                assert(narg == 3);
                return ph->value()[pk->cvalue()] =
                    this->arg(thread, l_vars, node, 2);
            }
        } else
            throw exception::value_type();
    } else
        throw exception::value_type();
}

/*** f_bool ******************************************************************/

template <impl::allocator A>
bool f_bool<A>::convert(typename basic_value<A>::value_ptr val)
{
    if (!val)
        throw exception::value_null();
    if (auto p = dynamic_cast<basic_value_bool<A>*>(val.get());
        p && !p->cvalue())
    {
        return false;
    } else
        return true;
}

template <impl::allocator A> typename basic_value<A>::value_ptr
f_bool<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    bool result = convert(this->arg(thread, l_vars, node, narg - 1));
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 2);
}

/*** f_clone *****************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_clone<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                 const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1)
        throw exception::op_narg();
    auto val = this->arg(thread, l_vars, node, 0);
    if (!val)
        throw exception::value_null();
    return val->shallow_copy(thread.get_allocator(), false);
}

/*** f_contains **************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_contains<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                    const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto a1 = this->arg(thread, l_vars, node, narg - 2);
    auto a2 = this->arg(thread, l_vars, node, narg - 1);
    if (!a1 || !a2)
        throw exception::value_null();
    auto hash = dynamic_cast<basic_value_hash<A>*>(a1.get());
    auto key = dynamic_cast<basic_value_string<A>*>(a2.get());
    if (!hash || !key)
        throw exception::value_type();
    auto result = hash->cvalue().contains(key->cvalue());
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 3);
}

/*** f_div_base **************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_div_base<A>::eval_impl(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                         const basic_code_node<A>& node, bool div)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto a1 = this->arg(thread, l_vars, node, narg - 2);
    auto a2 = this->arg(thread, l_vars, node, narg - 1);
    if (!a1 || !a2)
        throw exception::value_null();
    if (auto v1 = dynamic_cast<basic_value_int<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_int<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        auto s1 = v1->cvalue();
        auto s2 = v2->cvalue();
        if (s2 == 0)
            throw exception::op_div_zero();
        constexpr auto min = std::numeric_limits<config::value_int_type>::min();
        if (s1 == min && s2 == -1)
            throw exception::op_overflow();
        config::value_int_type result = div ? s1 / s2 : s1 % s2;
        return this->template make_result<basic_value_int<A>>(thread, l_vars,
                                            node, std::move(result), narg == 3);
    } else if (auto v1 = dynamic_cast<basic_value_unsigned<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_unsigned<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        uintmax_t u1 = v1->cvalue();
        uintmax_t u2 = v2->cvalue();
        if (u2 == 0)
            throw exception::op_div_zero();
        config::value_unsigned_type result = div ? u1 / u2 : u1 % u2;
        return this->template make_result<basic_value_unsigned<A>>(thread,
                                    l_vars, node, std::move(result), narg == 3);
    } else
        throw exception::value_type();
}

/*** f_div *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_div<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    return f_div_base<A>::eval_impl(thread, l_vars, node, true);
}

/*** f_eq ********************************************************************/

template <impl::allocator A>
bool f_eq<A>::compare(typename basic_value<A>::value_ptr val1,
                      typename basic_value<A>::value_ptr val2)
{
    if (!val1 || !val2)
        throw exception::value_null();
    if (auto v1 = dynamic_cast<basic_value_bool<A>*>(val1.get()))
        return v1->cvalue() == f_bool<A>::convert(val2);
    else if (auto v2 = dynamic_cast<basic_value_bool<A>*>(val2.get()))
        return f_bool<A>::convert(val1) == v2->cvalue();
    else if (auto v1 = dynamic_cast<basic_value_int<A>*>(val1.get())) {
        if (auto v2 = dynamic_cast<basic_value_int<A>*>(val2.get()))
            return v1->cvalue() == v2->cvalue();
        else if (auto v2 = dynamic_cast<basic_value_unsigned<A>*>(val2.get())) {
            if (v1->cvalue() < 0)
                return false;
            else
                return
                    config::value_unsigned_type(v1->cvalue()) == v2->cvalue();
        }
    } else if (auto v1 = dynamic_cast<basic_value_unsigned<A>*>(val1.get())) {
        if (auto v2 = dynamic_cast<basic_value_int<A>*>(val2.get())) {
            if (v2->cvalue() < 0)
                return false;
            else
                return
                    v1->cvalue() == config::value_unsigned_type(v2->cvalue());
        } else if (auto v2 = dynamic_cast<basic_value_unsigned<A>*>(val2.get()))
            return v1->cvalue() == v2->cvalue();
    } else if (auto v1 = dynamic_cast<basic_value_string<A>*>(val1.get()))
        if (auto v2 = dynamic_cast<basic_value_string<A>*>(val2.get()))
            return v1->cvalue() == v2->cvalue();
    throw exception::value_type();
}

template <impl::allocator A> typename basic_value<A>::value_ptr
f_eq<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto result = compare(this->arg(thread, l_vars, node, narg - 2),
                          this->arg(thread, l_vars, node, narg - 1));
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 3);
}

/*** f_erase *****************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_erase<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                 const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    auto container = this->arg(thread, l_vars, node, 0);
    if (!container)
        throw exception::value_null();
    if (narg == 1) {
        if (auto pv = dynamic_cast<basic_value_vector<A>*>(container.get()))
            pv->value().clear();
        else if (auto ph = dynamic_cast<basic_value_hash<A>*>(container.get()))
            ph->value().clear();
        else
            throw exception::value_type();
    } else {
        auto idx = this->arg(thread, l_vars, node, 1);
        if (!idx)
            throw exception::value_null();
        if (auto pv = dynamic_cast<basic_value_vector<A>*>(container.get())) {
            size_t i = 0;
            if (auto pi = dynamic_cast<basic_value_int<A>*>(idx.get())) {
                if (pi->cvalue() < 0)
                    throw exception::value_out_of_range();
                i = size_t(pi->cvalue());
            } else
                if (auto pi = dynamic_cast<basic_value_unsigned<A>*>(idx.get()))
                    i = size_t(pi->cvalue());
                else
                    throw exception::value_type();
            if (i < pv->cvalue().size())
                pv->value().resize(i);
        } else
            if (auto ph = dynamic_cast<basic_value_hash<A>*>(container.get())) {
                if (auto pk = dynamic_cast<basic_value_string<A>*>(idx.get()))
                    ph->value().erase(pk->cvalue());
                else
                    throw exception::value_type();
            } else
                throw exception::value_type();
    }
    return nullptr;
}

/*** f_ge ********************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_ge<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto result = !f_lt<A>::compare(this->arg(thread, l_vars, node, narg - 2),
                                    this->arg(thread, l_vars, node, narg - 1));
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 3);
}

/*** f_gt ********************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_gt<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto result = f_lt<A>::compare(this->arg(thread, l_vars, node, narg - 1),
                                   this->arg(thread, l_vars, node, narg - 2));
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 3);
}

/*** f_hash ******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_hash<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&,
                const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 0)
        throw exception::op_narg();
    auto pr = basic_value_hash<A>::create(thread.get_allocator());
    return pr;
}

/*** f_if ********************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_if<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    if (f_bool<A>::convert(this->arg(thread, l_vars, node, 0)))
        return this->arg(thread, l_vars, node, 1);
    else
        if (narg > 2)
            return this->arg(thread, l_vars, node, 2);
        else
            return nullptr;
}

/*** f_int *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_int<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
               const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    auto val = this->arg(thread, l_vars, node, narg - 1);
    if (!val)
        throw exception::value_null();
    config::value_int_type result = 0;
    if (auto v = dynamic_cast<basic_value_int<A>*>(val.get()))
        result = v->cvalue();
    else if (auto v = dynamic_cast<basic_value_unsigned<A>*>(val.get()))
        result = config::value_int_type(v->cvalue());
    else if (auto v = dynamic_cast<basic_value_string<A>*>(val.get()))
        result =
            std::get<config::value_int_type>(from_string(v->cvalue(), true));
    else
        throw exception::value_type();
    return this->template make_result<basic_value_int<A>>(thread, l_vars, node,
                                               std::move(result), narg == 2);
}

template <impl::allocator A>
std::variant<config::value_int_type, config::value_unsigned_type>
f_int<A>::from_string(const a_basic_string<A>& str, bool sign)
{
    bool negative = false;
    auto p = str.cbegin();
    auto e = str.cend();
    if (p == e)
        throw exception::value_bad();
    switch (*p) {
    case '-':
        if (!sign)
            throw exception::value_bad();
        negative = true;
        [[fallthrough]];
    case '+':
        ++p;
        break;
    default:
        break;
    }
    if (p == e)
        throw exception::value_bad();
    uintmax_t res = 0;
    bool overflow = false;
    for (; p != e; ++p)
        if (*p >= '0' && *p <= '9') {
            if (auto res2 = res * 10U + uintmax_t(*p - '0'); res2 >= res)
                res = res2;
            else // overflow, wraparound
                overflow = true; // do not throw immediately, check characters
                                 // after overflow
        } else
            throw exception::value_bad();
    if (overflow)
        throw exception::value_out_of_range();
    if (sign) {
        constexpr uintmax_t max =
            std::numeric_limits<config::value_int_type>::max();
        if (negative) {
            if (res <= max + 1U)
                return config::value_int_type(-res);
            else
                throw exception::value_out_of_range();
        } else
            if (res <= max)
                return config::value_int_type(res);
            else
                throw exception::value_out_of_range();
    } else
        if (res <= std::numeric_limits<config::value_unsigned_type>::max())
            return config::value_unsigned_type(res);
        else // handle config::value_unsigned_type smaller than uintmax_t
            throw exception::value_out_of_range();
}

/*** f_is_mt_safe ************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_is_mt_safe<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                      const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    auto val = this->arg(thread, l_vars, node, narg - 1);
    if (!val)
        throw exception::value_null();
    bool result = val->mt_safe();
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 2);
}

/*** f_is_null ***************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_is_null<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                   const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    bool result = !this->arg(thread, l_vars, node, narg - 1);
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 2);
}

/*** f_is_same ***************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_is_same<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                   const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto val1 = this->arg(thread, l_vars, node, narg - 2);
    auto val2 = this->arg(thread, l_vars, node, narg - 1);
    if (!val1 || !val2)
        throw exception::value_null();
    bool result = val1 == val2;
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 3);
}

/*** f_keys ******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_keys<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1)
        throw exception::op_narg();
    auto val = this->arg(thread, l_vars, node, 0);
    if (!val)
        throw exception::value_null();
    if (auto h = dynamic_cast<basic_value_hash<A>*>(val.get())) {
        auto result = basic_value_vector<A>::create(thread.get_allocator());
        for (auto&& [k, v]: h->cvalue()) {
            auto pk = basic_value_string<A>::create(thread.get_allocator());
            pk->value() = k;
            pk->set_mt_safe();
            result->value().push_back(std::move(pk));
        }
        std::sort(result->value().begin(), result->value().end(),
            [](auto&& a, auto&& b) {
                return static_cast<basic_value_string<A>*>(a.get())->cvalue() <
                static_cast<basic_value_string<A>*>(b.get())->cvalue();
            });
        return result;
    } else
        throw exception::value_type();
}

/*** f_le ********************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_le<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto result = !f_lt<A>::compare(this->arg(thread, l_vars, node, narg - 1),
                                    this->arg(thread, l_vars, node, narg - 2));
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 3);
}

/*** f_lt ********************************************************************/

template <impl::allocator A>
bool f_lt<A>::compare(typename basic_value<A>::value_ptr val1,
                      typename basic_value<A>::value_ptr val2)
{
    if (!val1 || !val2)
        throw exception::value_null();
    if (auto v1 = dynamic_cast<basic_value_bool<A>*>(val1.get()))
        return v1->cvalue() < f_bool<A>::convert(val2);
    else if (auto v2 = dynamic_cast<basic_value_bool<A>*>(val2.get()))
        return f_bool<A>::convert(val1) < v2->cvalue();
    else if (auto v1 = dynamic_cast<basic_value_int<A>*>(val1.get())) {
        if (auto v2 = dynamic_cast<basic_value_int<A>*>(val2.get()))
            return v1->cvalue() < v2->cvalue();
        else if (auto v2 = dynamic_cast<basic_value_unsigned<A>*>(val2.get())) {
            if (v1->cvalue() < 0)
                return true;
            else
                return
                    config::value_unsigned_type(v1->cvalue()) < v2->cvalue();
        }
    } else if (auto v1 = dynamic_cast<basic_value_unsigned<A>*>(val1.get())) {
        if (auto v2 = dynamic_cast<basic_value_int<A>*>(val2.get())) {
            if (v2->cvalue() < 0)
                return false;
            else
                return
                    v1->cvalue() < config::value_unsigned_type(v2->cvalue());
        } else if (auto v2 = dynamic_cast<basic_value_unsigned<A>*>(val2.get()))
            return v1->cvalue() < v2->cvalue();
    } else if (auto v1 = dynamic_cast<basic_value_string<A>*>(val1.get()))
        if (auto v2 = dynamic_cast<basic_value_string<A>*>(val2.get()))
            return v1->cvalue() < v2->cvalue();
    throw exception::value_type();
}

template <impl::allocator A> typename basic_value<A>::value_ptr
f_lt<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto result = compare(this->arg(thread, l_vars, node, narg - 2),
                          this->arg(thread, l_vars, node, narg - 1));
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 3);
}

/*** f_mod *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_mod<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    return f_div_base<A>::eval_impl(thread, l_vars, node, false);
}

/*** f_mt_safe ***************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_mt_safe<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                   const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1)
        throw exception::op_narg();
    auto val = this->arg(thread, l_vars, node, 0);
    if (!val)
        throw exception::value_null();
    try {
        val->set_mt_safe();
        return val;
    } catch (exception::value_mt_unsafe& e) {
        throw exception::value_mt_unsafe();
    }
}

/*** f_mul *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_mul<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto a1 = this->arg(thread, l_vars, node, narg - 2);
    auto a2 = this->arg(thread, l_vars, node, narg - 1);
    if (!a1 || !a2)
        throw exception::value_null();
    if (dynamic_cast<basic_value_string<A>*>(a2.get())) {
        using std::swap;
        swap(a1, a2); // now string is always the first argument
    }
    if (auto v1 = dynamic_cast<basic_value_string<A>*>(a1.get())) {
        config::value_unsigned_type n = 0;
        if (auto v2 = dynamic_cast<basic_value_int<A>*>(a2.get())) {
            if (v2->cvalue() < 0)
                throw exception::op_overflow();
            n = v2->cvalue();
        } else if (auto v2 = dynamic_cast<basic_value_unsigned<A>*>(a2.get()))
            n = v2->cvalue();
        else
            throw exception::value_type();
        a_basic_string<A> result;
        result.reserve(v1->cvalue().size() * n);
        for (decltype(n) i = 0; i < n; ++i)
            result.append(v1->cvalue());
        return this->template make_result<basic_value_string<A>>(thread, l_vars,
                                            node, std::move(result), narg == 3);
    } else if (auto v1 = dynamic_cast<basic_value_int<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_int<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        auto s1 = v1->cvalue();
        auto s2 = v2->cvalue();
        constexpr auto min = std::numeric_limits<config::value_int_type>::min();
        constexpr auto max = std::numeric_limits<config::value_int_type>::max();
        if (s1 != 0 && s2 != 0) {
            if (s1 > 0) {
                if (s2 > 0) {
                    if (s2 > max / s1)
                        throw exception::op_overflow();
                } else { // s2 < 0
                    if (s2 < min / s1)
                        throw exception::op_overflow();
                }
            } else { // s1 < 0
                if (s2 > 0) {
                    if (s1 < min / s2)
                        throw exception::op_overflow();
                } else { // s2 < 0
                    // -min is undefined in two's complement representation
                    // (which is required since C++20)
                    if (s1 == min || s2 == min || -s2 > max / -s1)
                        throw exception::op_overflow();
                }
            }
        }
        config::value_int_type result = s1 * s2;
        return this->template make_result<basic_value_int<A>>(thread, l_vars,
                                            node, std::move(result), narg == 3);
    } else if (auto v1 = dynamic_cast<basic_value_unsigned<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_unsigned<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        config::value_unsigned_type result =
            uintmax_t(v1->cvalue()) * uintmax_t(v2->cvalue());
        return this->template make_result<basic_value_unsigned<A>>(thread,
                                    l_vars, node, std::move(result), narg == 3);
    } else
        throw exception::value_type();
}

/*** f_ne ********************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_ne<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto result = !f_eq<A>::compare(this->arg(thread, l_vars, node, narg - 2),
                                    this->arg(thread, l_vars, node, narg - 1));
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 3);
}

/*** f_not *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_not<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
               const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    bool val = f_bool<A>::convert(this->arg(thread, l_vars, node, narg - 1));
    bool result = !val;
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                               std::move(result), narg == 2);
}

/*** f_or_base ***************************************************************/

template <impl::allocator A> bool
f_or_base<A>::eval_impl(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                        const basic_code_node<A>& node, size_t begin)
{
    for (size_t i = begin; i < this->narg(node); ++i)
        if (f_bool<A>::convert(this->arg(thread, l_vars, node, i)))
            return true;
    return false;
}

/*** f_or ********************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_or<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = this->eval_impl(thread, l_vars, node);
    return pr;
}

/*** f_or_r ******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_or_r<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg == 0)
        throw exception::op_narg();
    bool result = this->eval_impl(thread, l_vars, node, 1);
    return this->template make_result<basic_value_bool<A>>(thread, l_vars, node,
                                                       std::move(result), true);
}

/*** f_print *****************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_print<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                 const basic_code_node<A>& node, std::string_view)
{
    if (auto os = thread.std_out.value_or(thread.vm.std_out)) {
        std::osyncstream sync_os(*os);
        for (size_t i = 0; i < this->narg(node); ++i)
            if (auto p = this->arg(thread, l_vars, node, i))
                p->write(sync_os);
            else
                sync_os << "null";
    }
    return nullptr;
}

/*** f_seq *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_seq<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
               const basic_code_node<A>& node, std::string_view)
{
    typename basic_value<A>::value_ptr result = nullptr;
    for (size_t i = 0; i < this->narg(node); ++i)
        result = this->arg(thread, l_vars, node, i);
    return result;
}

/*** f_size ******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_size<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    auto val = this->arg(thread, l_vars, node, narg - 1);
    if (!val)
        throw exception::value_null();
    config::value_unsigned_type result = 1;
    if (auto v = dynamic_cast<basic_value_string<A>*>(val.get()))
        result = v->cvalue().size();
    else if (auto v = dynamic_cast<basic_value_vector<A>*>(val.get()))
        result = v->cvalue().size();
    else if (auto v = dynamic_cast<basic_value_hash<A>*>(val.get()))
        result = v->cvalue().size();
    return this->template make_result<basic_value_unsigned<A>>(thread, l_vars,
                                            node, std::move(result), narg == 2);
}

/*** f_sub *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_sub<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto a1 = this->arg(thread, l_vars, node, narg - 2);
    auto a2 = this->arg(thread, l_vars, node, narg - 1);
    if (!a1 || !a2)
        throw exception::value_null();
    if (auto v1 = dynamic_cast<basic_value_int<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_int<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        auto s1 = v1->cvalue();
        auto s2 = v2->cvalue();
        config::value_int_type result = uintmax_t(s1) - uintmax_t(s2);
        if ((s1 >= 0 && s2 < 0 && result < s1) ||
            (s1 < 0 && s2 >= 0 && result > s1))
        {
            throw exception::op_overflow();
        }
        return this->template make_result<basic_value_int<A>>(thread, l_vars,
                                            node, std::move(result), narg == 3);
    } else if (auto v1 = dynamic_cast<basic_value_unsigned<A>*>(a1.get())) {
        auto v2 = dynamic_cast<basic_value_unsigned<A>*>(a2.get());
        if (!v2)
            throw exception::value_type();
        config::value_unsigned_type result =
            uintmax_t(v1->cvalue()) - uintmax_t(v2->cvalue());
        return this->template make_result<basic_value_unsigned<A>>(thread,
                                    l_vars, node, std::move(result), narg == 3);
    } else
        throw exception::value_type();
}

/*** f_substr ****************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_substr<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                  const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg();
    auto a0 = this->arg(thread, l_vars, node, 0);
    auto a1 = this->arg(thread, l_vars, node, 1);
    auto a2 = this->arg(thread, l_vars, node, 2);
    if (!a0 || !a1 || (narg == 3 && !a2))
        throw exception::value_null();
    auto str = dynamic_cast<basic_value_string<A>*>(a0.get());
    if (!str)
        throw exception::value_type();
    size_t idx = 0;
    if (auto pi = dynamic_cast<basic_value_int<A>*>(a1.get())) {
        if (pi->cvalue() < 0)
            throw exception::value_out_of_range();
        idx = size_t(pi->cvalue());
    } else if (auto pi = dynamic_cast<basic_value_unsigned<A>*>(a1.get()))
        idx = size_t(pi->cvalue());
    else
        throw exception::value_type();
    size_t len = std::string::npos;
    if (a2) {
        if (auto pl = dynamic_cast<basic_value_int<A>*>(a2.get())) {
            if (pl->cvalue() < 0)
                throw exception::value_out_of_range();
            len = size_t(pl->cvalue());
        } else if (auto pl = dynamic_cast<basic_value_unsigned<A>*>(a2.get()))
            len = size_t(pl->cvalue());
        else
            throw exception::value_type();
    }
    auto pr = basic_value_string<A>::create(thread.get_allocator());
    if (idx < str->cvalue().size())
        pr->value() = str->cvalue().substr(idx, len);
    return pr;
}

/*** f_type ******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_type<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    auto val = this->arg(thread, l_vars, node, narg - 1);
    if (!val)
        throw exception::value_null();
    a_basic_string<A> result{val->type_name(), thread.get_allocator()};
    return this->template make_result<basic_value_string<A>>(thread, l_vars,
                                         node, std::move(result), narg == 2);
}

/*** f_unsigned **************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_unsigned<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&l_vars,
                    const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    auto val = this->arg(thread, l_vars, node, narg - 1);
    if (!val)
        throw exception::value_null();
    config::value_unsigned_type result = 0U;
    if (auto v = dynamic_cast<basic_value_unsigned<A>*>(val.get()))
        result = v->cvalue();
    else if (auto v = dynamic_cast<basic_value_int<A>*>(val.get()))
        result = config::value_unsigned_type(v->cvalue());
    else if (auto v = dynamic_cast<basic_value_string<A>*>(val.get()))
        result = std::get<config::value_unsigned_type>(
                                    f_int<A>::from_string(v->cvalue(), false));
    else
        throw exception::value_type();
    return this->template make_result<basic_value_unsigned<A>>(thread, l_vars,
                                           node, std::move(result), narg == 2);
}

/*** f_var *******************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_var<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
               const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 1 && narg != 2)
        throw exception::op_narg();
    auto arg_name = this->arg(thread, l_vars, node, 0);
    if (!arg_name)
        throw exception::value_null();
    auto name = dynamic_cast<basic_value_string<A>*>(arg_name.get());
    if (!name)
        throw exception::value_type();
    if (narg == 1) {
        if (auto v = l_vars.lookup(name->cvalue()))
            return *v;
        else
            throw exception::unknown_symbol(name->cvalue());
    } else {
        auto v = this->arg(thread, l_vars, node, 1);
        l_vars.insert(name->cvalue(), v);
        return v;
    }
}

/*** f_vector ****************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_vector<A>::eval(basic_state<A>& thread, basic_symbol_table<A>&,
                  const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 0)
        throw exception::op_narg();
    auto pr = basic_value_vector<A>::create(thread.get_allocator());
    return pr;
}

/*** f_while *****************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_while<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                 const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2)
        throw exception::op_narg();
    typename basic_value<A>::value_ptr result = nullptr;
    while (f_bool<A>::convert(this->arg(thread, l_vars, node, 0)))
        result = this->arg(thread, l_vars, node, 1);
    return result;
}

} // namespace predef

template <impl::allocator A>
std::shared_ptr<basic_symbol_table<A>> predef_symbols(const A& alloc)
{
    auto p = std::allocate_shared<basic_symbol_table<A>>(alloc, alloc, nullptr);
    return add_predef_symbols(p, true);
}

template <impl::allocator A> std::shared_ptr<basic_symbol_table<A>>
add_predef_symbols(std::shared_ptr<basic_symbol_table<A>> sym, bool replace)
{
    static const std::array factory(std::to_array<
        std::pair<a_basic_string<A>,
            typename basic_value<A>::value_ptr(*)(const A&)>
    >({
        { "add", predef::f_add<A>::create },
        { "and", predef::f_and<A>::template create<predef::f_and<A>> },
        { "and_r", predef::f_and<A>::template create<predef::f_and_r<A>> },
        { "at", predef::f_at<A>::create },
        { "bool", predef::f_bool<A>::create },
        { "clone", predef::f_clone<A>::create },
        { "contains", predef::f_contains<A>::create },
        { "div", predef::f_div<A>::template create<predef::f_div<A>> },
        { "eq", predef::f_eq<A>::create },
        { "erase", predef::f_erase<A>::create },
        { "ge", predef::f_ge<A>::create },
        { "gt", predef::f_gt<A>::create },
        { "hash", predef::f_hash<A>::create },
        { "if", predef::f_if<A>::create },
        { "int", predef::f_int<A>::create },
        { "is_mt_safe", predef::f_is_mt_safe<A>::create },
        { "is_null", predef::f_is_null<A>::create },
        { "is_same", predef::f_is_same<A>::create },
        { "keys", predef::f_keys<A>::create },
        { "le", predef::f_le<A>::create },
        { "lt", predef::f_lt<A>::create },
        { "mod", predef::f_mod<A>::template create<predef::f_mod<A>> },
        { "mt_safe", predef::f_mt_safe<A>::create },
        { "mul", predef::f_mul<A>::create },
        { "ne", predef::f_ne<A>::create },
        { "not", predef::f_not<A>::create },
        { "or", predef::f_or<A>::template create<predef::f_or<A>> },
        { "or_r", predef::f_or<A>::template create<predef::f_or_r<A>> },
        { "print", predef::f_print<A>::create },
        { "seq", predef::f_seq<A>::create },
        { "size", predef::f_size<A>::create },
        { "sub", predef::f_sub<A>::create },
        { "substr", predef::f_substr<A>::create },
        { "type", predef::f_type<A>::create },
        { "unsigned", predef::f_unsigned<A>::create },
        { "var", predef::f_var<A>::create },
        { "vector", predef::f_vector<A>::create },
        { "while", predef::f_while<A>::create },
    }));
    if (sym) {
        for (auto&& f: factory) {
            if (replace || !sym->contains(f.first))
                sym->insert(f.first, f.second(sym->get_allocator()));
        }
    }
    return sym;
}

} // namespace threadscript
