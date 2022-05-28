#pragma once

/*! \file
 * \brief Storage of parsed script code
 */

#include "threadscript/allocated.hpp"
#include "threadscript/concepts.hpp"
#include "threadscript/vm_data.hpp"

namespace threadscript {

//! A single node in a tree representing a parsed script
/*! \note Raw pointers are used for child nodes, so that an addional copy of
 * allocator \a A does not have to be stored in each node or pointer. */
template <impl::allocator A> class basic_code_node {
public:
    using node_ptr = basic_code_node*;
    using const_node_ptr = const basic_code_node*;
    basic_code_node(const basic_code_node&) = delete;
    basic_code_node(basic_code_node&&) = delete;
    basic_code_node& operator=(const basic_code_node&) = delete;
    basic_code_node& operator=(basic_code_node&&) = delete;
    ~basic_code_node();
private:
    basic_code_node(const file_location& location, std::string_view name);
    file_location location; //!< Location in the script source file
    a_basic_string<A> name; //!< Node name
    a_basic_vector<node_ptr, A> children; //!< Child nodes
    //! Value of the node
    /*! It is not \c std::nullopt if either the node \ref name has been
     * resolved, or if a fixed value is assigned to the node by the script
     * source code. If it is \c std::nullopt, then \ref name will be resolved
     * to a value using a symbol_table. */
    std::optional<typename basic_value<A>::value_ptr> value;
};

//! The representation of a single parsed script file
template <impl::allocator A> class basic_script {
public:
private:
    a_basic_string<A> file; //!< The script file name
    //! The root node of the parsed script
    typename basic_code_node<A>::node_ptr root;
};

} //namespace threadscript
