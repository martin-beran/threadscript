/*! \file
 * \brief The implementation part of threadscript/syntax.hpp
 */

#include "threadscript/syntax.hpp"
#include "threadscript/syntax_canon.hpp"

namespace threadscript {

/*** syntax_base *************************************************************/

syntax_base::~syntax_base() = default;

void syntax_base::parse(script_builder& builder, std::string_view src,
                        std::string_view file)
{
    // TODO
}

/*** syntax_factory **********************************************************/

std::map<std::string_view, std::unique_ptr<syntax_base>(*)()>
syntax_factory::_registry {
    { "canon", []() { return std::make_unique<syntax::canon>(); } },
};

std::unique_ptr<syntax_base> syntax_factory::create(std::string_view syntax) {
    if (auto it = _registry.find(syntax); it != _registry.end())
        return it->second();
    else
        return nullptr;
}

} // namespace threadscript
