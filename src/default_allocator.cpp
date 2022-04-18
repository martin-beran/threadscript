/*! \file
 * \brief Implementation part of default_allocator.hpp
 */

#include "threadscript/config.hpp"
#include "threadscript/default_allocator.hpp"

namespace threadscript {

/*** allocator_config ********************************************************/

bool allocator_config::allocate(size_t size) noexcept
{
    auto l_balance{_limits.balance.load()};
    if (l_balance != limits_t::unlimited_size &&
        _metrics.balance + size > l_balance)
    {
        ++_metrics.alloc_rejects;
        return false;
    }
    ++_metrics.alloc_ops;
    auto allocs{++_metrics.allocs};
    auto balance{_metrics.balance += size};
    if (allocs > _metrics.max_allocs)
        _metrics.max_allocs = allocs;
    if (balance > _metrics.max_balance)
        _metrics.max_balance = balance;
    return true;
}

void allocator_config::deallocate(size_t size) noexcept
{
    ++_metrics.dealloc_ops;
    --_metrics.allocs;
    _metrics.balance -= size;
}

} // namespace threadscript
