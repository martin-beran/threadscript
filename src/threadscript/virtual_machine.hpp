#pragma once

/*! \file
 * \brief Main classes of the ThreadScript virtual machine
 */

#include "threadscript/configure.hpp"
#include "threadscript/symbol_table.hpp"
#include "threadscript/vm_data.hpp"

#include <atomic>
#include <cassert>
#include <iostream>

namespace threadscript {

template <impl::allocator A> class basic_code_node;
template <impl::allocator A> class basic_script;
template <impl::allocator A> class basic_value_function;

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
    //! The default maximum stack depth
    static constexpr size_t default_max_stack = 1000;
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
    //! Global variables shared by all threads
    /*! It is a shared pointer to a symbol table, so that the global symbol
     * table can be replaced without synchronization of all thread. Existing
     * threads will continue to use the old symbol table until they request the
     * new one. */
    std::atomic<std::shared_ptr<const basic_symbol_table<A>>> sh_vars;
    //! Used as the standard output stream.
    /*! If \c nullptr, standard output is discarded. It can be overriden
     * for a thread by basic_state::std_out. The user of this stream must
     * ensure proper synchronization, e.g., by using std::osyncstream. */
    std::atomic<std::ostream*> std_out = &std::cout;
private:
    //! The allocator used by this VM.
    [[no_unique_address]] A alloc;
    //! The number of basic_state objects attached to this VM
    std::atomic<size_t> _num_states{0};
    //! Needs access to num_states
    friend class basic_state<A>;
};

//! The state of a single thread in a basic_virtual_machine
/*! \tparam A the allocator type used by this thread
 * \threadsafe{safe,unsafe}
 * \test in file test_virtual_machine.cpp */
template <impl::allocator A> class basic_state {
public:
    using allocator_type = A; //!< The allocator type used by this class
    //! The type of the virtual machine containing this state.
    using vm_t = basic_virtual_machine<A>;
    //! The constructor registers basic_state in \a vm.
    /*! \param[in] vm the virtual machine which this state is attached to. */
    explicit basic_state(vm_t& vm):
        vm(vm), alloc(vm.get_allocator()),
        t_vars(vm.get_allocator(), vm.sh_vars.load().get())
    {
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
    //! Gets the stack trace generated from the current \ref stack.
    /*! If creation of the stack trace throws an exception, an empty stack
     * trace is returned.
     * \return the stack trace */
    [[nodiscard]] stack_trace current_stack() const noexcept;
    vm_t& vm; //!< The virtual machine
    //! Used as the standard output stream.
    /*! If \c nullptr, standard output is discarded. If \c std::nullopt, then
     * \c vm.std_out is used instead. The user of this stream must ensure
     * proper synchronization if the same stream is used by multiple threads,
     * e.g., by using std::osyncstream. */
    std::optional<std::ostream*> std_out;
private:
    //! A stack frame
    /*! For simplicity, it uses frame_location, which does not use custom
     * allocators. The memory used by a stack is indirectly limited by
     * max_stack
     * \todo Use custom allocators for stack frames. */
    struct stack_frame {
        //! Creates a stack frame.
        /*! \param[in] alloc the allocator used by this stack frame
         * \param[in] thread the thread containing this fram */
        explicit stack_frame(const A& alloc, basic_state& thread):
            l_vars(alloc, &thread.t_vars) {}
        frame_location location; //!< Location in the script source
        basic_symbol_table<A> l_vars; //!< Local variables of this stack frame
    };
    //! A stack
    /*! The outermost function level (the bottom of the stack) is at index 0.
     * The innermost function level (the top of the stack, the most recently
     * called function) is at \c back().
     * Adding and removing stack frames must be done by push_frame() and
     * pop_frame(). */
    using stack_t = a_basic_deque<stack_frame, A>;
    //! Pushes a new stack frame to \ref stack.
    /*! \param[in] frame the new stack frame
     * \return a reference to the pushed stack frame
     * \throw exception::op_recursion if the new frame would exceed max_stack */
    stack_frame& push_frame(stack_frame&& frame);
    //! Pops the top element from the stack.
    /*! It handles freeing memory consumed by the stack. The stack must not be
     * empty. */
    void pop_frame() noexcept;
    //! The allocator used by this state
    [[no_unique_address]] A alloc;
    //! The maximum stack depth for this thread
    size_t max_stack = basic_virtual_machine<A>::default_max_stack;
    //! The stack of this thread
    stack_t stack;
    //! Global variables of this thread
    basic_symbol_table<A> t_vars;
    //! basic_code_node::eval() needs access to basic_state
    friend class basic_code_node<A>;
    //! basic_script::eval() needs access to basic_state
    friend class basic_script<A>;
    //! basic_value_function::eval() needs access to basic_state
    friend class basic_value_function<A>;
};

} // namespace threadscript
