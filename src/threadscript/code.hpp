#pragma once

/*! \file
 * \brief Storage of parsed script code
 */

#include "threadscript/allocated.hpp"
#include "threadscript/concepts.hpp"
#include "threadscript/vm_data.hpp"
#include <memory>

namespace threadscript {

template <impl::allocator A> class basic_script;
template <impl::allocator A> class basic_value_script;
template <impl::allocator A> class basic_value_function;
template <class Derived, impl::allocator A> class basic_value_native_fun;

//! A single node in a tree representing a parsed script
/*! \tparam A the allocator type
 * \threadsafe{safe, unsafe}
 * \note Raw pointers are used for child nodes internally, so that an
 * additional copy of allocator \a A does not have to be stored in each node or
 * pointer. */
template <impl::allocator A> class basic_code_node {
    struct tag {}; //!< Used to control access to a public constructor
public:
    using allocator_type = A; //!< The allocator type used by this class
    //! Type of a value of the node
    /*! It is not \c std::nullopt if either the node \ref name has been
     * resolved, or if a fixed value is assigned to the node by the script
     * source code. If it is \c std::nullopt, then \ref name will be resolved
     * to a value using a symbol_table. */
    using value_t = std::optional<typename basic_value<A>::value_ptr>;
    //! Number of spaces added on each indentation level of write().
    static constexpr size_t indent_step = 4;
    //! Constructor, used by basic_script
    /*! \param[in] t an ignored parameter used to prevent using this
     * constructor directly
     * \param[in] alloc the allocator used by members
     * \param[in] file the file name of the owner script
     * \param[in] location the location in the current script file,
     * represented by the owher basic_script
     * \param[in] name the name of this node
     * \param[in] value the value of this node */
    basic_code_node(tag t, const A& alloc, const a_basic_string<A>& file,
                    const file_location& location, std::string_view name,
                    value_t value);
    //! No copying
    basic_code_node(const basic_code_node&) = delete;
    //! No moving
    basic_code_node(basic_code_node&&) = delete;
    //! No copying
    basic_code_node& operator=(const basic_code_node&) = delete;
    //! No moving
    basic_code_node& operator=(basic_code_node&&) = delete;
    //! It recursively deletes _children.
    /*! It reuses the allocator of _children for deleting child nodes,
     * therefore another allocator need not be stored. */
    ~basic_code_node();
    //! Test of equality.
    /*! It is intended mainly for testing that a parser produces the expected
     * representation of a script. It compares the node trees recursively.
     * Members \ref value are compared only for being both \c std::nullopt,
     * both \c nullptr, or both non-null pointers, but the values are not
     * compared for equality.
     * \param[in] o another node
     * \return whether \c this and \a o are equal
     * \todo Compare \ref value for equality */
    bool operator==(const basic_code_node& o) const noexcept;
    //! Writes the node to a stream.
    /*! It is used by operator<<(std::ostream&, const basic_code_node<A>&).
     * \param[in] os an output stream
     * \param[in] indent the number of spaces used for indentation */
    void write(std::ostream& os, size_t indent) const;
private:
    //! A private (raw) pointer to a code node
    using priv_ptr = basic_code_node*;
    //! A private (raw) pointer to a const code node
    using priv_const_ptr = const basic_code_node*;
    //! Evaluates the node and returns the result.
    /*! If \ref value is \c std::nullopt, it is first resolved using \a lookup
     * and \ref name. Then, the (resolved) value is evaluated by its
     * basic_value::eval().
     * \param[in] thread the current thread
     * \param[in] l_vars  the symbol table of the current stack frame;
     see parameter \a l_vars of basic_value::eval() for more information
     * \return the result of evaluation
     * \throw a class derived from exception::base if evaluation fails; other
     * exceptions are wrapped in exception::wrapped */
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                        basic_symbol_table<A>& l_vars) const;
    //! The script file name
    /*! It always references basic_script::_file of the owner script. */
    const a_basic_string<A>& _file;
    file_location location; //!< Location in the script source file
    a_basic_string<A> name; //!< Node name
    //! Child nodes
    /*! Member are never \c nullptr. */
    a_basic_vector<priv_ptr, A> _children;
    value_t value; //!< Value of the node
    //! The owner basic_script needs access to node internals
    friend class basic_script<A>;
    //! basic_value_function::eval() needs access to node internals
    friend class basic_value_function<A>;
    //! basic_value_native_fun needs access to _children
    template <class Derived, impl::allocator Alloc>
        friend class basic_value_native_fun;
};

//! Writes a textual description of the node to a stream
/*! It is intended mainly for testing that a parser produces the expected
 * representation of a script.
 * \param[in] os an output stream
 * \param[in] node the node to be written
 * \return \a os */
template <impl::allocator A>
std::ostream& operator<<(std::ostream& os, const basic_code_node<A>& node);

//! Writes a textual description of the node to a stream
/*! It is intended mainly for testing that a parser produces the expected
 * representation of a script.
 * \param[in] os an output stream
 * \param[in] script the script to be written
 * \return \a os */
template <impl::allocator A>
std::ostream& operator<<(std::ostream& os, const basic_script<A>& script);

//! The representation of a single parsed script file
/*! It owns a tree of basic_code_node objects representing the whole parsed
 * content of the file
 * \tparam A the allocator type
 * \threadsafe{safe, unsafe} */
template <impl::allocator A> class basic_script:
    public std::enable_shared_from_this<basic_script<A>>
{
    struct tag {}; //!< Used to control access to a public constructor
public:
    using allocator_type = A; //!< The allocator type used by this class
    //! A shared pointer to the script object
    using script_ptr = std::shared_ptr<basic_script>;
    //! The type of individual nodes of the parsed script
    using node_type = basic_code_node<A>;
    //! A publicly available pointer to a code node
    /*! It points to a basic_code_node, but manages its owner basic_script.
     * This reduces the memory overhead of each basic_code_node object. */
    using node_ptr = std::shared_ptr<const node_type>;
    //! Creates an empty script representation, with null root().
    /*! \param[in] t an ignored parameter used to prevent using this
     * constructor directly
     * \param[in] alloc the allocator, whose copy is stored in \ref alloc and
     * used for all allocations made by this object
     * \param[in] file a script file name stored in the basic_script object */
    basic_script(tag t, const A& alloc, std::string_view file);
    //! No copying
    basic_script(const basic_script&) = delete;
    //! No moving
    basic_script(basic_script&&) = delete;
    //! No copying
    basic_script& operator=(const basic_script&) = delete;
    //! No moving
    basic_script& operator=(basic_script&&) = delete;
    //! Recursively destroys all child nodes stored in root()
    ~basic_script();
    //! Creates an empty script representation.
    /*! \param[in] alloc the allocator, whose copy is stored in \ref alloc and
     * used for all allocations made by this object
     * \param[in] file a script file name stored in the created object
     * \return a shared pointer to the created script object */
    static script_ptr create(const A& alloc, std::string_view file) {
        return std::allocate_shared<basic_script>(alloc, tag{}, alloc, file);
    }
    //! Adds a new node of this script.
    /*! \param[in] parent the created node will be added as the _root node if
     * \c nullptr, and as the last child of \a parent otherwise
     * \param[in] location the location of the new node in the script
     * \param[in] name the name of the new node
     * \param[in] value the value of the new node
     * \return the new created node
     * \throw exception::parse_error if \a parent is \c nullptr and a _root
     * node already exists */
    node_ptr add_node(const node_ptr& parent, const file_location& location,
                      std::string_view name,
                      typename node_type::value_t value = std::nullopt);
    //! Evaluates the script and returns the result.
    /*! If _root is \c nullptr, then \c nullptr is returned. Otherwise, the
     * root node is evaluated by calling its basic_code_node::eval().
     * Evaluating a script adds a new stack frame, therefore outside any
     * function, local variables refer to a script-local symbol table.
     * This symbol table is returned in \a l_vars, providing access to any
     * symbols (variables and functions) defined during the script execution.
     * \param[in] thread the current thread
     * \param[out] l_vars if not \c nullptr and the script finishes normally
     * (does not throw an exception), its local symbol table (from its stack
     * frame) will be moved here
     * \return the result of evaluation
     * \throw a class derived from exception::base if evaluation fails; other
     * exceptions are wrapped in exception::wrapped */
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
                                basic_symbol_table<A>* l_vars = nullptr) const;
    //! Gets the allocator used by this script.
    /*! \return a copy of the allocator object */
    [[nodiscard]] A get_allocator() const noexcept { return alloc; }
    //! Gets the script file name.
    /*! \return the name */
    [[nodiscard]] const a_basic_string<A>& file() const noexcept {
        return _file;
    }
    //! Test of equality.
    /*! It is intended mainly for testing that a parser produces the expected
     * representation of a script. It compares the node trees recursively.
     * \param[in] o another script
     * \return whether \c this and \a o are equal */
    bool operator==(const basic_script& o) const noexcept;
private:
    //! The script file name
    a_basic_string<A> _file;
    //! The allocator used to allocate nodes
    [[no_unique_address]] A alloc;
    //! The root node of the parsed script
    typename node_type::priv_ptr _root = nullptr;
    //! <tt>operator<< \<A>()</tt> needs access to private members.
    friend std::ostream& operator<< <A>(std::ostream&, const basic_script<A>&);
    //! basic_value_script needs access to _root
    friend class basic_value_script<A>;
};

//! Common handling of funtion calls
/*! This class provides common functionality needed by basic_value_function and
 * classes derived from basic_value_native_fun. It handles adjusting the stack
 * and processing function call arguments.
 * \todo use basic_call_context for common handling of function calls in
 * basic_value_function and classes derived from basic_value_native_fun */
template <impl::allocator A> class basic_call_context {
    // TODO
};

namespace impl {
//! The name of basic_value_function
inline constexpr char name_value_function[] = "function";
//! The base class of basic_value_function
/*! \tparam A an allocator type */
template <allocator A> using basic_value_function_base =
    basic_typed_value<basic_value_function<A>,
        typename basic_script<A>::node_ptr, name_value_function, A>;
} //namespace impl

//! The value class holding a reference to a script function
template <impl::allocator A> class basic_value_function final:
    public impl::basic_value_function_base<A>
{
    static_assert(
        !impl::uses_allocator<typename basic_value_function::value_type, A>);
    using impl::basic_value_function_base<A>::basic_value_function_base;
public:
    //! Name in the local symbol table referencing a vector of parameters
    /*! The vector is constructed from function call arguments and inserted to
     * the newly created local symbol table when the function is called. */
    static constexpr std::string_view symbol_params{"_args"};
protected:
    //! Calls the referenced function.
    /*! It returns \c nullptr if the internal pointer to a function
     * implementation basic_code_node is \c nullptr.
     * \copydetails basic_value::eval() */
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
        basic_symbol_table<A>& l_vars, const basic_code_node<A>& node,
        std::string_view fun_name) override;
};

namespace impl {
//! The name of basic_value_script
inline constexpr char name_value_script[] = "script";
//! The base class of basic_value_script
/*! \tparam A an allocator type */
template <allocator A> using basic_value_script_base =
    basic_typed_value<basic_value_script<A>,
        std::shared_ptr<basic_script<A>>, name_value_script, A>;
} //namespace impl

//! The value class holding a reference to a script
template <impl::allocator A> class basic_value_script final:
    public impl::basic_value_script_base<A>
{
    static_assert(
        !impl::uses_allocator<typename basic_value_script::value_type, A>);
    using impl::basic_value_script_base<A>::basic_value_script_base;
protected:
    //! Runs the referenced script.
    /*! If there is no associated basic_script \c nullptr is returned.
     * Otherwise, basic_script::eval() of the associated script is called.
     * \copydetails basic_value::eval() */
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
        basic_symbol_table<A>& l_vars, const basic_code_node<A>& node,
        std::string_view fun_name) override;
};

namespace impl {
//! The name of basic_value_native_fun
inline constexpr char name_value_native_fun[] = "native_fun";
//! An empty value
/*! Used as the basic_typed_value::value_type type of basic_value_native_fun */
struct empty {};
//! The base class of basic_value_native_fun
/*! \tparam A an allocator type */
template <class D, allocator A> using basic_value_native_fun_base =
    basic_typed_value<basic_value_native_fun<D, A>, empty,
        name_value_native_fun, A>;
} // namespace impl

//! The value class holding a reference to a function implemented by native C++
/*! This is the abstract base class for native functions. Each derived type
 * should override member function eval().
 * \tparam Derived the derived native function type
 * \tparam A the allocator type used by the function */
template <class Derived, impl::allocator A> class basic_value_native_fun:
    public impl::basic_value_native_fun_base<Derived, A>
{
    static_assert(
        !impl::uses_allocator<typename basic_value_native_fun::value_type, A>);
    using impl::basic_value_native_fun_base<Derived, A>::
        basic_value_native_fun_base;
public:
    //! Stores the allocator
    /*! \param[in] t an ignored parameter that prevents using this constructor
     * directly
     * \param[in] alloc an allocator to be used by this object */
    basic_value_native_fun(typename basic_value_native_fun::tag t,
                           const A& alloc);
    //! Creates the \a Derived native function object
    /*! \param[in] alloc an allocator to be used by the created object
     * \return the created object */
    static typename basic_value<A>::value_ptr create(const A& alloc);
protected:
    //! Evaluates the value and returns the result.
    /*! The default implementation in basic_value_native_fun does nothing and
     * returns \c nullptr. Overriding definitions in derived classes implement
     * individual native functions and return appropriate values.
     *
     * \copydetails basic_value::eval() */
    typename basic_value<A>::value_ptr eval(basic_state<A>& thread,
        basic_symbol_table<A>& l_vars, const basic_code_node<A>& node,
        std::string_view fun_name) override;
    //! Gets the number of arguments.
    /*! It is intended to be called from eval().
     * \param[in] node the argument \a node of eval()
     * \return the number of function arguments. */
    size_t narg(const basic_code_node<A>& node) const noexcept {
        return node._children.size();
    }
    //! Evaluates an argument and returns its value.
    /*! It is intended to be called from eval().
     * \param[in] thread the argument \a thread of the caller eval()
     * \param[in] l_vars the argument \a l_vars of the caller eval()
     * \param[in] node the argument \a node of the caller eval()
     * \param[in] idx the (zero-based) index of the argument
     * \return the argument value; \c nullptr if \a idx is greater than the
     * index of the last argument */
    typename basic_value<A>::value_ptr arg(basic_state<A>& thread,
                                           basic_symbol_table<A>& l_vars,
                                           const basic_code_node<A>& node,
                                           size_t idx);
    //! The allocator used by this native function
    A alloc;
};

} //namespace threadscript
