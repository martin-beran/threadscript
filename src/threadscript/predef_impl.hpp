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
            } catch (exception::value_read_only& e) {
                throw exception::value_read_only(thread.current_stack());
            }
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
    for (size_t i = 0; i < this->narg(node); ++i)
        this->arg(thread, l_vars, node, i);
    return nullptr;
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
        { "print", predef::f_print<A>::create },
        { "seq", predef::f_seq<A>::create },
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
