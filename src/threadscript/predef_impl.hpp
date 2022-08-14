#pragma once

/*! \file
 * \brief The implementation part of predef.hpp
 */

#include "threadscript/predef.hpp"

#include <syncstream>

namespace threadscript {

namespace predef {

/*** f_print *****************************************************************/

template <impl::allocator A>
typename basic_value<A>::value_ptr f_print<A>::eval(
    basic_state<A>& thread,
    const basic_symbol_table<A>& lookup,
    const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& sym,
    const basic_code_node<A>& node, std::string_view)
{
    if (auto os = thread.std_out.value_or(thread.vm.std_out)) {
        std::osyncstream sync_os(*os);
        for (size_t i = 0; i < this->narg(node); ++i)
            sync_os << this->arg(thread, lookup, sym, node, i);
    }
    return nullptr;
}

/*** f_seq *******************************************************************/

template <impl::allocator A>
typename basic_value<A>::value_ptr f_seq<A>::eval(
    basic_state<A>& thread,
    const basic_symbol_table<A>& lookup,
    const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& sym,
    const basic_code_node<A>& node, std::string_view)
{
    for (size_t i = 0; i < this->narg(node); ++i)
        this->arg(thread, lookup, sym, node, i);
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
