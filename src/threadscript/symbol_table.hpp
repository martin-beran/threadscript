#pragma once

/*! \file
 * \brief A symbol table, that is, a mapping from names to basic_value.
 */

#include "threadscript/vm_data.hpp"
#include <type_traits>

namespace threadscript {

//! A symbol table
/*! This class is used to store mappings from variable names to values. Names
 * are strings (a_basic_string), values are shared pointers to basic_value. A
 * symbol table keeps a pointer to an optional parent table, used for a lookup
 * (recursively) if a name is not found in this table. If a table does not have
 * a parent, a lookup fails when a name is not found in this table.
 * \tparam A an allocator type
 * \note The parent is referenced by a plain pointer, because it is expected
 * that the hierarchy of virtual machine / thread / function ensures that a
 * parent symbol table always outlives its child symbol tables.
 * \test in file test_symbol_table.cpp */
template <impl::allocator A> class basic_symbol_table {
    //! Checks that the move constructor and assignments are \c noexcept
    /*! \note These static asserts cannot be placed directly in symbol_table,
     * because the compiler would report an incomplete type when evaluating
     * them. */
    struct nothrow_check {
        static_assert(std::is_nothrow_move_constructible_v<basic_symbol_table>);
        static_assert(std::is_nothrow_move_assignable_v<basic_symbol_table>);
    };
public:
    using key_type = a_basic_string<A>; //!< Key type, which is a name
    using value_type = typename basic_value<A>::value_ptr; //!< Value type
    //! The type used as a storage for contents of the symbol table
    using storage = a_basic_hash<key_type, value_type, A>;
    //! Creates a new empty symbol table
    /*! \param[in] alloc an allocator used by this table
     * \param[in] parent the parent table, or \c nullptr if this is a top-level
     * parent table */
    basic_symbol_table(const A& alloc, const basic_symbol_table* parent):
        data(alloc), parent(parent) {}
    //! Copy constructor
    /*! \param[in] o the source object */
    basic_symbol_table(const basic_symbol_table& o) = default;
    //! Move constructor
    /*! \param[in] o the source object */
    // NOLINTNEXTLINE(hicpp-noexcept-move): noexcept inferred
    basic_symbol_table(basic_symbol_table&& o) = default;
    //! Default destructor
    ~basic_symbol_table() = default;
    //! Copy assignment
    /*! \param[in] o the source object
     * \return \c *this */
    basic_symbol_table& operator=(const basic_symbol_table& o) = default;
    //! Move assignment
    /*! \param[in] o the source object
     * \return \c *this */
    // NOLINTNEXTLINE(hicpp-noexcept-move): noexcept inferred
    basic_symbol_table& operator=(basic_symbol_table&& o) = default;
    //! Gets the parent table.
    /*! \return the parent symbol table or \c nullptr */
    const basic_symbol_table* parent_table() const noexcept { return parent; }
    //! Gets writable access to the internal storage object.
    /*! It is intended mainly for operation over multiple symbols, e.g.,
     * iteration of all symbols. It should be also called by all member
     * functions that delete symbols from the table, because it handles
     * automatic rehashing if the load factor decreases significantly.
     * \return the internal storage object */
    storage& symbols() {
        if (data.load_factor() <= data.max_load_factor() / 3)
            data.rehash(data.size() / data.max_load_factor() / 2 * 3);
        return data;
    }
    //! \copydoc csymbols()
    const storage& symbols() const noexcept { return data; }
    //! Gets read-only access to the internal storage object.
    /*! It is intended mainly for operation over multiple symbols, e.g.,
     * iteration of all symbols.
     * \return contents of this symbol table */
    const storage& csymbols() const noexcept { return data; }
    //! Checks if the symbol table contains a symbol.
    /*! \param[in] name a symbol name
     * \param[in] use_parent whether to search the chain of parent tables if
     * the symbol is not found in this table
     * \return \c true if the symbol is contained in this table or in a table
     * accessed by the chain of parent_table(); \c false otherwise */
    bool contains(const key_type& name, bool use_parent = true) const;
    //! Finds a symbol.
    /*! \param[in] name a symbol name
     * \param[in] use_parent whether to search the chain of parent tables if
     * the symbol is not found in this table
     * \return the symbol value if the symbol exists in this table or in a
     * table accessed by the chain of parent_table(); \c std::nullopt
     * otherwise*/
    std::optional<value_type> lookup(const key_type& name,
                                     bool use_parent = true) const;
    //! Assings a value to a symbol name.
    /*! It creates a new symbol if it does not exist. It operates on this table
     * only, it does not access parent_table().
     * \param[in] name a symbol name
     * \param[in] value a symbol value
     * \return \c true if a new symbol has been added, \c false if symbol \a
     * name has already existed */
    bool insert(key_type name, value_type value);
    //! Deletes a symbol (a name and a value).
    /*! It operates on this table only, it does not access parent_table().
     * \param[in] name a symbol name
     * \return \c true if the symbol has been deleted, \c if symbol does not
     * exist in the table. */
    bool erase(const key_type& name);
private:
    storage data; //!< Contents of the symbol table
    //! The optional parent symbol table
    const basic_symbol_table* parent = nullptr;
};

} // namespace threadscript
