#pragma once

/*! \file
 * \brief The implementation part of predef.hpp
 */

#include "threadscript/predef.hpp"

#include <syncstream>

namespace threadscript {

namespace predef {

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
    auto a0 = this->arg(thread, l_vars, node, 0);
    if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get())) {
        pr->value() = result;
        return a0;
    }
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = result;
    return pr;
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
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get())) {
            pr->value() = result;
            return a0;
        }
    }
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = result;
    return pr;
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
    if (narg == 3) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get())) {
            pr->value() = result;
            return a0;
        }
    }
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = result;
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
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get())) {
            pr->value() = result;
            return a0;
        }
    }
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = result;
    return pr;
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
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get())) {
            pr->value() = result;
            return a0;
        }
    }
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = result;
    return pr;
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
    if (narg == 3) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get())) {
            pr->value() = result;
            return a0;
        }
    }
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = result;
    return pr;
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
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get())) {
            pr->value() = result;
            return a0;
        }
    }
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = result;
    return pr;
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
    auto a0 = this->arg(thread, l_vars, node, 0);
    if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get())) {
        pr->value() = result;
        return a0;
    }
    auto pr = basic_value_bool<A>::create(thread.get_allocator());
    pr->value() = result;
    return pr;
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
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_string<A>*>(a0.get())) {
            pr->value() = val->type_name();
            return a0;
        }
    }
    auto pr = basic_value_string<A>::create(thread.get_allocator());
    pr->value() = val->type_name();
    return pr;
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
        { "and", predef::f_and<A>::template create<predef::f_and<A>> },
        { "and_r", predef::f_and<A>::template create<predef::f_and_r<A>> },
        { "bool", predef::f_bool<A>::create },
        { "clone", predef::f_clone<A>::create },
        { "eq", predef::f_eq<A>::create },
        { "if", predef::f_if<A>::create },
        { "is_mt_safe", predef::f_is_mt_safe<A>::create },
        { "is_null", predef::f_is_null<A>::create },
        { "is_same", predef::f_is_same<A>::create },
        { "mt_safe", predef::f_mt_safe<A>::create },
        { "not", predef::f_not<A>::create },
        { "or", predef::f_or<A>::template create<predef::f_or<A>> },
        { "or_r", predef::f_or<A>::template create<predef::f_or_r<A>> },
        { "print", predef::f_print<A>::create },
        { "seq", predef::f_seq<A>::create },
        { "type", predef::f_type<A>::create },
        { "var", predef::f_var<A>::create },
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
