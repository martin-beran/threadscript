/*! \file
 * \brief The implementation part of threadscript/exception.hpp
 */

#include "threadscript/exception.hpp"

#include <cassert>
#include <cstring>

namespace threadscript {

/*** file_location ***********************************************************/

std::string file_location::to_string() const
{
    return (line == unknown ? std::string{} : std::to_string(line)) + ":" +
        (column == unknown ? std::string{} : std::to_string(column));
}

/*** src_location ************************************************************/

std::string src_location::to_string() const
{
    return file + ":" + file_location::to_string();
}

/*** frame_location **********************************************************/

std::string frame_location::to_string() const
{
    return src_location::to_string() + ":" + function + "()";
}

/*** stack_trace *************************************************************/

const int stack_trace::full_idx = std::ios_base::xalloc();

std::ostream& stack_trace::full(std::ostream& os)
{
    os.iword(full_idx) = 1;
    return os;
}

bool stack_trace::full_stream(std::ostream& os) {
    auto& f = os.iword(full_idx);
    bool result = f != 0;
    f = 0;
    return result;
}

std::string stack_trace::to_string(bool full) const
{
    std::string result;
    for (size_t i = 0; i < size(); ++i) {
        result.append("    ").append(std::to_string(i)).append(". ").
            append(at(i).to_string()).append("\n");
        if (!full)
            break;
    }
    return result;
}

namespace exception {

/*** base ********************************************************************/

std::string base::make_msg(const std::string& msg, const stack_trace& trace)
{
    return (trace.empty() ?
            std::string{} : trace.front().to_string() + ": ") + msg;
}

void base::set_msg(size_t sz)
{
    const char* p = what();
    size_t pl = strlen(p);
    assert(pl >= sz);
    _msg = std::string_view(p + pl - sz, sz);
}

void base::set_trace(stack_trace trace)
{
    _trace = std::move(trace);
    std::string saved_msg{msg()};
    // intentional object slicing used to set what()
    static_cast<std::runtime_error&>(*this) =
        std::runtime_error(make_msg(saved_msg, _trace));
    set_msg(saved_msg.size());
}

std::string base::to_string(bool full) const {
    std::string result = what();
    if (full)
        result.append("\n").append(_trace.to_string(true));
    return result;
}

/*** wrapped *****************************************************************/

wrapped::wrapped(std::string_view msg, stack_trace trace):
    base(std::string{msg}, std::move(trace)), _wrapped(std::current_exception())
{
    assert(_wrapped);
}

} // namespace exception

} // namespace threadscript
