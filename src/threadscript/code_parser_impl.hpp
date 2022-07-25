#pragma once

/*! \file
 * \brief The implementation part of threadscript/code_parser.hpp
 */

#include "threadscript/code_parser.hpp"
#include "threadscript/code_builder_impl.hpp"

namespace threadscript {

template <impl::allocator A> basic_script<A>::script_ptr
parse_code(const A& alloc, std::string_view src, std::string_view file,
           std::string_view syntax)
{
    basic_script_builder_impl<A> builder(alloc);
    auto parser = syntax_factory::create(syntax);
    parser->parse(builder, src, file);
    return builder.get_script();
}

template <impl::allocator A> basic_script<A>::script_ptr
parse_code_file(const A& alloc, std::string_view file, std::string_view syntax)
{
    basic_script_builder_impl<A> builder(alloc);
    auto parser = syntax_factory::create(syntax);
    parser->parse_file(builder, file);
    return builder.get_script();
}

} // namespace threadscript
