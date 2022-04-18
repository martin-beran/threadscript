#pragma once

/*! \file
 * \brief Main classes of the ThreadScript virtual machine
 */

#include "threadscript/config.hpp"
#include "threadscript/config_default.hpp"

#include <atomic>
#include <cassert>

namespace threadscript {

template <class Allocator> class basic_state;

//! The ThreadScript virtual machine
/*! An object of this class represents a single instance of the ThreadScript
 * engine. It can handle multiple basic_state objects, representing individual
 * threads running in this VM. All handled states must be destroyed before
 * destroying the VM.
 * \tparam Allocator the allocator used by this VM
 * \threadsafe{safe,safe} */
template <class Allocator> class basic_virtual_machine {
public:
    //! Default constructor
    /*! \param[in] alloc the allocator to be used by this VM */
    explicit basic_virtual_machine(Allocator alloc): alloc(std::move(alloc)) {}
    //! No copying
    basic_virtual_machine(const basic_virtual_machine&) = delete;
    //! No moving
    basic_virtual_machine(basic_virtual_machine&&) = delete;
    //! The destructor checks that no basic_state refers this VM.
    ~basic_virtual_machine() {
        assert(_num_states == 0);
    }
    //! No copying
    basic_virtual_machine& operator=(const basic_virtual_machine&) = delete;
    //! No moving
    basic_virtual_machine& operator=(basic_virtual_machine&&) = delete;
    //! Gets the allocator used by this VM.
    /*! \return a copy of the allocator object */
    Allocator get_allocator() const noexcept { return alloc; }
    //! Gets the number of states (threads) attached to this VM.
    /*! \return the number of attached basic_state objects */
    [[nodiscard]] config::size_type num_states() const noexcept {
        return _num_states;
    }
private:
    //! The allocator used by this VM.
    Allocator alloc;
    //! The number of basic_state objects attached to this VM
    std::atomic<config::size_type> _num_states{0};
    //! Needs access to num_states
    friend class basic_state<Allocator>;
};

//! The state of a single thread in a basic_virtual_machine
/*! \tparam Allocator the allocator type used by this thread; it must be the
 * same as the allocator used by the VM
 * \threadsafe{safe,unsafe} */
template <class Allocator> class basic_state {
public:
    //! The type of the virtual machine containing this state.
    using vm_t = basic_virtual_machine<Allocator>;
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
    Allocator get_allocator() const noexcept { return alloc; }
    vm_t& vm; //!< The virtual machine 
private:
    Allocator alloc; //!< The allocator used by this state
};

} // namespace threadscript
