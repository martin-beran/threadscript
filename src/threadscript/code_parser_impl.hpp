#pragma once

/*! \file
 * \brief The implementation part of threadscript/code_parser.hpp
 */

#include "threadscript/code_parser.hpp"
#include "threadscript/code_builder_impl.hpp"

namespace threadscript {

template <impl::allocator A> basic_script<A>::script_ptr
parse_code(const A& alloc, std::string_view src, std::string_view file,
           std::string_view syntax, parser::context::trace_t trace)
{
    basic_script_builder_impl<A> builder(alloc);
    auto parser = syntax_factory::create(syntax);
    if (!parser)
        throw exception::parse_error("Unknown syntax \"" +
                                     std::string(syntax) + "\"");
    parser->parse(builder, src, file, std::move(trace));
    return builder.get_script();
}

template <impl::allocator A> basic_script<A>::script_ptr
parse_code_stream(const A& alloc, std::istream& is, std::string_view file,
                  std::string_view syntax, parser::context::trace_t trace)
{
    basic_script_builder_impl<A> builder(alloc);
    auto parser = syntax_factory::create(syntax);
    if (!parser)
        throw exception::parse_error("Unknown syntax \"" +
                                     std::string(syntax) + "\"");
    parser->parse_stream(builder, is, file, std::move(trace));
    return builder.get_script();
}

template <impl::allocator A> basic_script<A>::script_ptr
parse_code_file(const A& alloc, std::string_view file, std::string_view syntax,
                parser::context::trace_t trace)
{
    basic_script_builder_impl<A> builder(alloc);
    auto parser = syntax_factory::create(syntax);
    if (!parser)
        throw exception::parse_error("Unknown syntax \"" +
                                     std::string(syntax) + "\"");
    parser->parse_file(builder, file, std::move(trace));
    return builder.get_script();
}

} // namespace threadscript
