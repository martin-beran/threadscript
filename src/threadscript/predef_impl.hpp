#pragma once

/*! \file
 * \brief The implementation part of predef.hpp
 */

#include "predef.hpp"

namespace threadscript {

namespace predef {

/*** f_print *****************************************************************/

template <impl::allocator A>
typename basic_value<A>::value_ptr f_print<A>::eval(
    basic_state<A>& thread,
    const basic_symbol_table<A>& lookup,
    const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& sym,
    const basic_code_node<A>& node, std::string_view fun_name)
{
    if (auto os = thread.std_out.value_or(thread.vm.std_out)) {
        std::osyncstream sync_os(*os);
        for (size_t i = 0; i < narg(node); ++i)
            sync_os << arg(thread, lookup, sym, node, i);
    }
    return nullptr;
}

/*** f_seq *******************************************************************/

template <impl::allocator A>
typename basic_value<A>::value_ptr f_seq<A>::eval(
    basic_state<A>& thread,
    const basic_symbol_table<A>& lookup,
    const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& sym,
    const basic_code_node<A>& node, std::string_view fun_name)
{
    for (size_t i = 0; i < narg(node); ++i)
        arg(thread, lookup, sym, node, i);
    return nullptr;
}

} // namespace predef

template <impl::allocator A>
std::shared_ptr<basic_symbol_table<A>> predef_symbols(const A& /*alloc*/)
{
    // TODO
    return nullptr;
}

template <impl::allocator A> std::shared_ptr<basic_symbol_table<A>>
add_predef_symbols(std::shared_ptr<basic_symbol_table<A>> sym, bool /*replace*/)
{
    if (sym) {
        // TODO
    }
    return sym;
}

} // namespace threadscript
