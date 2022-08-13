#pragma once

/*! \file
 * \brief The implementation part of predef.hpp
 */

#include "predef.hpp"

namespace threadscript {

namespace predef {
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
