#pragma once

/*! \file
 * \brief The implementation part of predef.hpp
 */

#include "threadscript/predef.hpp"

#include <syncstream>

namespace threadscript {

namespace predef {

/*** f_bool ******************************************************************/

template <impl::allocator A>
bool f_bool<A>::convert(basic_state<A>& thread,
                        typename basic_value<A>::value_ptr val)
{
    if (!val)
        throw exception::value_null(thread.current_stack());
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
        throw exception::op_narg(thread.current_stack());
    bool result = convert(thread, this->arg(thread, l_vars, node, narg - 1));
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get()))
            try {
                pr->value() = result;
                return a0;
            } catch (exception::value_read_only&) {
                throw exception::value_read_only(thread.current_stack());
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
        throw exception::op_narg(thread.current_stack());
    auto val = this->arg(thread, l_vars, node, 0);
    if (!val)
        throw exception::value_null(thread.current_stack());
    return val->shallow_copy(thread.get_allocator(), false);
}

/*** f_if ********************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
f_if<A>::eval(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
              const basic_code_node<A>& node, std::string_view)
{
    size_t narg = this->narg(node);
    if (narg != 2 && narg != 3)
        throw exception::op_narg(thread.current_stack());
    if (f_bool<A>::convert(thread, this->arg(thread, l_vars, node, 0)))
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
        throw exception::op_narg(thread.current_stack());
    auto val = this->arg(thread, l_vars, node, narg - 1);
    if (!val)
        throw exception::value_null(thread.current_stack());
    bool result = val->mt_safe();
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get()))
            try {
                pr->value() = result;
                return a0;
            } catch (exception::value_read_only&) {
                throw exception::value_read_only(thread.current_stack());
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
        throw exception::op_narg(thread.current_stack());
    bool result = !this->arg(thread, l_vars, node, narg - 1);
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get()))
            try {
                pr->value() = result;
                return a0;
            } catch (exception::value_read_only&) {
                throw exception::value_read_only(thread.current_stack());
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
        throw exception::op_narg(thread.current_stack());
    auto val1 = this->arg(thread, l_vars, node, narg - 2);
    auto val2 = this->arg(thread, l_vars, node, narg - 1);
    if (!val1 || !val2)
        throw exception::value_null(thread.current_stack());
    bool result = val1 == val2;
    if (narg == 3) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_bool<A>*>(a0.get()))
            try {
                pr->value() = result;
                return a0;
            } catch (exception::value_read_only&) {
                throw exception::value_read_only(thread.current_stack());
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
        throw exception::op_narg(thread.current_stack());
    auto val = this->arg(thread, l_vars, node, 0);
    if (!val)
        throw exception::value_null(thread.current_stack());
    try {
        val->set_mt_safe();
        return val;
    } catch (exception::value_mt_unsafe& e) {
        throw exception::value_mt_unsafe(thread.current_stack());
    }
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
        throw exception::op_narg(thread.current_stack());
    auto val = this->arg(thread, l_vars, node, narg - 1);
    if (!val)
        throw exception::value_null(thread.current_stack());
    if (narg == 2) {
        auto a0 = this->arg(thread, l_vars, node, 0);
        if (auto pr = dynamic_cast<basic_value_string<A>*>(a0.get()))
            try {
                pr->value() = val->type_name();
                return a0;
            } catch (exception::value_read_only& e) {
                throw exception::value_read_only(thread.current_stack());
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
        throw exception::op_narg(thread.current_stack());
    auto arg_name = this->arg(thread, l_vars, node, 0);
    if (!arg_name)
        throw exception::value_null(thread.current_stack());
    auto name = dynamic_cast<basic_value_string<A>*>(arg_name.get());
    if (!name)
        throw exception::value_type(thread.current_stack());
    if (narg == 1) {
        if (auto v = l_vars.lookup(name->cvalue()))
            return *v;
        else
            throw exception::unknown_symbol(name->cvalue(),
                                            thread.current_stack());
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
        throw exception::op_narg(thread.current_stack());
    typename basic_value<A>::value_ptr result = nullptr;
    while (f_bool<A>::convert(thread, this->arg(thread, l_vars, node, 0)))
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
        { "bool", predef::f_bool<A>::create },
        { "clone", predef::f_clone<A>::create },
        { "if", predef::f_if<A>::create },
        { "is_mt_safe", predef::f_is_mt_safe<A>::create },
        { "is_null", predef::f_is_null<A>::create },
        { "is_same", predef::f_is_same<A>::create },
        { "mt_safe", predef::f_mt_safe<A>::create },
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
