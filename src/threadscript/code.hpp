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
/*! \note Raw pointers are used for child nodes internally, so that an addional
 * copy of allocator \a A does not have to be stored in each node or pointer.
 */
template <impl::allocator A> class basic_code_node:
    public std::enable_shared_from_this<basic_code_node<A>> {
public:
    using node_ptr = std::shared_ptr<const basic_code_node>;
    basic_code_node(const basic_code_node&) = delete;
    basic_code_node(basic_code_node&&) = delete;
    basic_code_node& operator=(const basic_code_node&) = delete;
    basic_code_node& operator=(basic_code_node&&) = delete;
    ~basic_code_node();
private:
    using priv_ptr = basic_code_node*;
    using priv_const_ptr = const basic_code_node*;
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
    friend class basic_script<A>;
};

//! The representation of a single parsed script file
template <impl::allocator A> class basic_script {
    struct tag {}; //!< Used to control access to a public constructor
public:
    using allocator_type = A; //!< The allocator type used by this class
    using script_ptr = std::shared_ptr<basic_script>;
    using node_type = basic_code_node<A>;
    basic_script(tag t, std::string_view file);
    basic_script(const basic_script&) = delete;
    basic_script(basic_script&&) = delete;
    basic_script& operator=(const basic_script&) = delete;
    basic_script& operator=(basic_script&&) = delete;
    ~basic_script();
    static script_ptr create(const A& alloc) {
        return std::allocate_shared<basic_script>(alloc, tag{}, alloc);
    }
private:
    a_basic_string<A> file; //!< The script file name
    [[no_unique_address]] A alloc; //!< The allocator used to allocate nodes
    //! The root node of the parsed script
    typename node_type::priv_ptr root;
};

} //namespace threadscript
