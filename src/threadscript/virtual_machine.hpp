#pragma once

/*! \file
 * \brief Main classes of the ThreadScript virtual machine
 */

#include "threadscript/config.hpp"
#include "threadscript/vm_data.hpp"

#include <atomic>
#include <cassert>

namespace threadscript {

template <impl::allocator A> class basic_state;

//! The ThreadScript virtual machine
/*! An object of this class represents a single instance of the ThreadScript
 * engine. It can handle multiple basic_state objects, representing individual
 * threads running in this VM. All handled states must be destroyed before
 * destroying the VM.
 *
 * In order to allow passing ownership of allocations among various objects
 * belonging to a VM, all allocators related to the same VM must cooperate.
 * It must be possible to allocate and deallocate each object by any allocator,
 * or its (possibly rebound) copy, in the VM. Allocators of type
 * default_allocator sharing the same allocator_config satisfy this
 * requirement.
 * \tparam A the allocator used by this VM
 * \threadsafe{safe,safe}
 * \test in file test_virtual_machine.cpp */
template <impl::allocator A> class basic_virtual_machine {
public:
    using allocator_type = A; //!< The allocator type used by this class
    //! Default constructor
    /*! \param[in] alloc the allocator to be used by this VM */
    // \NOLINTNEXTLINE(modernize-pass-by-value)
    explicit basic_virtual_machine(const A& alloc): alloc(alloc) {}
    //! No copying
    basic_virtual_machine(const basic_virtual_machine&) = delete;
    //! No moving
    basic_virtual_machine(basic_virtual_machine&&) = delete;
    //! The destructor checks that no basic_state refers this VM.
    ~basic_virtual_machine() {
        assert(_num_states.load() == 0);
    }
    //! No copying
    basic_virtual_machine& operator=(const basic_virtual_machine&) = delete;
    //! No moving
    basic_virtual_machine& operator=(basic_virtual_machine&&) = delete;
    //! Gets the allocator used by this VM.
    /*! \return a copy of the allocator object */
    [[nodiscard]] A get_allocator() const noexcept { return alloc; }
    //! Gets the number of states (threads) attached to this VM.
    /*! \return the number of attached basic_state objects */
    [[nodiscard]] size_t num_states() const noexcept {
        return _num_states;
    }
private:
    //! The allocator used by this VM.
    [[no_unique_address]] A alloc;
    //! The number of basic_state objects attached to this VM
    std::atomic<size_t> _num_states{0};
    //! Needs access to num_states
    friend class basic_state<A>;
};

//! The state of a single thread in a basic_virtual_machine
/*! \tparam A the allocator type used by this thread; it must be the
 * same as the allocator used by the VM
 * \threadsafe{safe,unsafe}
 * \test in file test_virtual_machine.cpp */
template <impl::allocator A> class basic_state {
public:
    using allocator_type = A; //!< The allocator type used by this class
    //! The type of the virtual machine containing this state.
    using vm_t = basic_virtual_machine<A>;
    //! The constructor registers basic_state in \a vm.
    /*! \param[in] vm the virtual machine which this state is attached to. */
    explicit basic_state(vm_t& vm): vm(vm), alloc(vm.get_allocator()) {
        ++vm._num_states;
    }
    //! No copying
    basic_state(const basic_state&) = delete;
    //! No moving
    basic_state(basic_state&&) = delete;
    //! The destructor unregisters basic_state from \a vm.
    ~basic_state() {
        --vm._num_states;
    }
    //! No copying
    basic_state& operator=(const basic_state&) = delete;
    //! No moving
    basic_state& operator=(basic_state&&) = delete;
    //! Get the allocator used by this state.
    /*! \return a copy of the allocator object */
    [[nodiscard]] A get_allocator() const noexcept { return alloc; }
    vm_t& vm; //!< The virtual machine 
private:
    [[no_unique_address]] A alloc; //!< The allocator used by this state
};

} // namespace threadscript
