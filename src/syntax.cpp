/*! \file
 * \brief The implementation part of threadscript/syntax.hpp
 */

#include "threadscript/syntax.hpp"
#include "threadscript/code_builder.hpp"
#include "threadscript/syntax_canon.hpp"

#include <fstream>

namespace threadscript {

/*** syntax_base *************************************************************/

syntax_base::~syntax_base() = default;

void syntax_base::parse(script_builder& builder, std::string_view src,
                        std::string_view file, parser::context::trace_t trace)
{
    builder.create_script(file);
    run_parser(builder, src, std::move(trace));
}

void syntax_base::parse_file(script_builder& builder, std::string_view file,
                             parser::context::trace_t trace)
{
    std::ifstream is{std::string{file}};
    is.exceptions(std::ifstream::failbit); // throw if open failed
    is.exceptions(std::ifstream::goodbit); // do not throw on empty file
    std::stringbuf sb;
    is >> &sb; // read file, C++ streams cannot report errors here
    std::string src = std::move(sb).str();
    parse(builder, src, file, std::move(trace));
}

/*** syntax_factory **********************************************************/

namespace {

//! Create a parser object
/*! \tparam T a parser type, derived from syntax_base
 * \return the created parser object */
template <std::derived_from<syntax_base> T>
std::unique_ptr<syntax_base> make_syntax()
{
    return std::make_unique<T>();
}

} // namespace

std::map<std::string_view, syntax_factory::make_fun> syntax_factory::_registry {
    { syntax_canon, &make_syntax<syntax::canon> },
};

std::unique_ptr<syntax_base> syntax_factory::create(std::string_view syntax) {
    if (auto it = _registry.find(syntax); it != _registry.end())
        return it->second();
    else
        return nullptr;
}

} // namespace threadscript
