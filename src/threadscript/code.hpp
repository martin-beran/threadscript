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
public:
    //! A publicly available pointer to a code node
    /*! It points to a basic_code_node, but manages its owner basic_script.
     * This reduces the memory overhead of each basic_code_node object. */
    using node_ptr = std::shared_ptr<const basic_code_node>;
    //! No copying
    basic_code_node(const basic_code_node&) = delete;
    //! No moving
    basic_code_node(basic_code_node&&) = delete;
    //! No copying
    basic_code_node& operator=(const basic_code_node&) = delete;
    //! No moving
    basic_code_node& operator=(basic_code_node&&) = delete;
    //! Recursively destroys any child nodes stored in \ref children
    ~basic_code_node();
private:
    //! A private (raw) pointer to a code node
    using priv_ptr = basic_code_node*;
    //! A private (raw) pointer to a const code node
    using priv_const_ptr = const basic_code_node*;
    //! Constructor, used by basic_script
    /*! \param[in] location the location in the current script file,
     * represented by the owher basic_script
     * \param[in] name the name of this node */
    basic_code_node(const file_location& location, std::string_view name);
    file_location location; //!< Location in the script source file
    a_basic_string<A> name; //!< Node name
    a_basic_vector<priv_ptr, A> children; //!< Child nodes
    //! Value of the node
    /*! It is not \c std::nullopt if either the node \ref name has been
     * resolved, or if a fixed value is assigned to the node by the script
     * source code. If it is \c std::nullopt, then \ref name will be resolved
     * to a value using a symbol_table. */
    std::optional<typename basic_value<A>::value_ptr> value;
    //! The owner basic_script needs access to node internals
    friend class basic_script<A>;
};

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
    //! Creates an empty script representation, with null \ref root.
    /*! \param[in] t an ignored parameter used to overload constructors and to
     * prevent using this constructor directly
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
    //! Recursively destroys all child nodes stored in \ref root
    ~basic_script();
    //! Creates an empty script representation.
    /*! \param[in] alloc the allocator, whose copy is stored in \ref alloc and
     * used for all allocations made by this object
     * \param[in] file a script file name stored in the created object
     * \return a shared pointer to the created script object */
    static script_ptr create(const A& alloc, std::string_view file) {
        return std::allocate_shared<basic_script>(alloc, tag{}, alloc, file);
    }
private:
    //! The script file name
    a_basic_string<A> file;
    //! The allocator used to allocate nodes
    [[no_unique_address]] A alloc;
    //! The root node of the parsed script
    typename node_type::priv_ptr root;
};

} //namespace threadscript
