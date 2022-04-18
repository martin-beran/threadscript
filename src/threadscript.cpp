/*! \file
 * \brief The implementation part of threadscript.hpp
 */

#include "threadscript/threadscript.hpp"

namespace threadscript {

template class basic_virtual_machine<vm_allocator>;
template class basic_state<vm_allocator>;

} // namespace threadscript
