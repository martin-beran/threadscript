/*! \file
 * \brief The implementation part of threadscript/exception.hpp
 */

#include "threadscript/exception.hpp"

#include <cassert>
#include <cstring>

namespace threadscript {

/*** src_location ************************************************************/

std::string src_location::to_string() const
{
    return file + ":" +
        (line == unknown ? std::string{} : std::to_string(line)) + ":" +
        (column == unknown ? std::string{} : std::to_string(column));
}

/*** frame_location **********************************************************/

std::string frame_location::to_string() const
{
    return src_location::to_string() + function + "()";
}

namespace exception {

/*** base ********************************************************************/

void base::set_msg(size_t sz)
{
    const char* p = what();
    size_t pl = strlen(p);
    assert(pl >= sz);
    _msg = std::string_view(p + pl - sz, sz);
}

/*** wrapped *****************************************************************/

wrapped::wrapped(std::string_view msg, stack_trace trace):
    base(std::string{msg}, std::move(trace)), _wrapped(std::current_exception())
{
    assert(_wrapped);
}

} // namespace exception

} // namespace threadscript
