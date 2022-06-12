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

//! A single node in a tree representing a parsed script
/*! \tparam A the allocator type
 * \note Raw pointers are used for child nodes internally, so that an addional
 * copy of allocator \a A does not have to be stored in each node or pointer.
 */
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
     * \param[in] location the location in the current script file,
     * represented by the owher basic_script
     * \param[in] name the name of this node
     * \param[in] value the value of this node */
    basic_code_node(tag t, const A& alloc, const file_location& location,
                    std::string_view name, value_t value);
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
    file_location location; //!< Location in the script source file
    a_basic_string<A> name; //!< Node name
    //! Child nodes
    /*! Member are never \c nullptr. */
    a_basic_vector<priv_ptr, A> _children;
    value_t value; //!< Value of the node
    //! The owner basic_script needs access to node internals
    friend class basic_script<A>;
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
 * \tparam A the allocator type */
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
    //! Gets the allocator used by this script.
    /*! \return a copy of the allocator object */
    [[nodiscard]] A get_allocator() const noexcept { return alloc; }
    //! Gets the script file name.
    /*! \return the name */
    [[nodiscard]] const a_basic_string<A>& file() const noexcept {
        return _file;
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
};

} //namespace threadscript
