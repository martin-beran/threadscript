/*! \file
 * \brief The implementation part of threadscript.hpp
 */

#include "threadscript/threadscript.hpp"

namespace threadscript {

template class basic_virtual_machine<allocator_any>;
template class basic_state<allocator_any>;

template class value<allocator_any>;

} // namespace threadscript
