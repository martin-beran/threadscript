/*! \file
 * \brief Implementation part of default_allocator.hpp
 */

#include "threadscript/default_allocator.hpp"

namespace threadscript {

/*** allocator_config ********************************************************/

bool allocator_config::allocate(size_t size) noexcept
{
    auto l_balance{_limits.balance.load(std::memory_order_relaxed)};
    if (l_balance != limits_t::unlimited_size &&
        _metrics.balance.load(std::memory_order_relaxed) + size > l_balance)
    {
        _metrics.alloc_rejects.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    _metrics.alloc_ops.fetch_add(1, std::memory_order_relaxed);
    auto allocs{_metrics.allocs.fetch_add(1, std::memory_order_relaxed) + 1};
    auto balance{
        _metrics.balance.fetch_add(size, std::memory_order_relaxed) + size
    };
    if (allocs > _metrics.max_allocs.load(std::memory_order_relaxed))
        _metrics.max_allocs.store(allocs, std::memory_order_relaxed);
    if (balance > _metrics.max_balance.load(std::memory_order_relaxed))
        _metrics.max_balance.store(balance, std::memory_order_relaxed);
    return true;
}

void allocator_config::deallocate(size_t size) noexcept
{
    _metrics.dealloc_ops.fetch_add(1, std::memory_order_relaxed);
    _metrics.allocs.fetch_sub(1, std::memory_order_relaxed);
    _metrics.balance.fetch_sub(size, std::memory_order_relaxed);
}

} // namespace threadscript
