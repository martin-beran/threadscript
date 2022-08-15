#pragma once

/*! \file
 * \brief Implementation of threadscript::script_builder
 */

#include "threadscript/code_builder.hpp"

#include <cassert>

namespace threadscript {

//! An implementation of script_builder for a particular allocator type
/*! \tparam A an allocator type used to allocate script nodes */
template <impl::allocator A>
class basic_script_builder_impl: public script_builder {
public:
    //! Creates a new script_builder.
    /*! It stores a copy of \a alloc, which will be used to allocate the script
     * object and its nodes.
     * \param[in] alloc the allocator for this script */
    basic_script_builder_impl(const A& alloc): alloc(alloc) {}
    void create_script(std::string_view file) override;
    //! Gets the created and stored script object.
    /*! Calling this function without a previous call to create_script() causes
     * a failed assert.
     * \return the script object; never \c nullptr */
    basic_script<A>::script_ptr get_script() const noexcept {
        assert(_script);
        return _script;
    }
    node_handle add_node(const node_handle& parent,
                         const file_location& location,
                         std::string_view name,
                         const value_handle& value = {}) override;
    value_handle create_value_bool(bool val) override;
    value_handle create_value_int(config::value_int_type val) override;
    value_handle create_value_unsigned(config::value_unsigned_type val)
        override;
    value_handle create_value_string(std::string_view val) override;
    //! Converts a type-erased basic_script::node_ptr to a correct type.
    /*! \param[in] hnd the type-erased node handler
     * \return the converted node pointer */
    static basic_script<A>::node_ptr cast(const node_handle& hnd) noexcept {
        return
            std::reinterpret_pointer_cast<const basic_code_node<A>>(get(hnd));
    }
    //! Converts a type-erased basic_value::value_ptr to a correct type.
    /*! \param[in] hnd the type-erased value handler
     * \return the converted value pointer */
    static std::optional<typename basic_value<A>::value_ptr>
    cast(const value_handle& hnd) noexcept {
        if (auto opt = get(hnd))
            return std::reinterpret_pointer_cast<basic_value<A>>(*opt);
        else
            return std::nullopt;
    }
private:
    A alloc; //!< The stored allocator
    basic_script<A>::script_ptr _script; //!< The stored created script object
};

/*** basic_script_builder_impl ***********************************************/

template <impl::allocator A>
auto basic_script_builder_impl<A>::add_node(const node_handle& parent,
                                            const file_location& location,
                                            std::string_view name,
                                            const value_handle& value)
    -> node_handle
{
    assert(_script);
    auto p = _script->add_node(cast(parent), location, name, cast(value));
    node_handle result;
    get(result) = p;
    return result;
}

template <impl::allocator A>
void basic_script_builder_impl<A>::create_script(std::string_view file)
{
    assert(!_script);
    _script = basic_script<A>::create(alloc, file);
}

template <impl::allocator A>
auto basic_script_builder_impl<A>::create_value_bool(bool val)
    -> value_handle
{
    auto p = basic_value_bool<A>::create(alloc);
    p->value() = val;
    p->set_mt_safe(); // This value originated from a literal, must be constant
    value_handle result;
    get(result) = p;
    return result;
}

template <impl::allocator A>
auto basic_script_builder_impl<A>::create_value_int(config::value_int_type val)
    -> value_handle
{
    auto p = basic_value_int<A>::create(alloc);
    p->value() = val;
    p->set_mt_safe(); // This value originated from a literal, must be constant
    value_handle result;
    get(result) = p;
    return result;
}

template <impl::allocator A>
auto basic_script_builder_impl<A>::create_value_string(std::string_view val)
    -> value_handle
{
    auto p = basic_value_string<A>::create(alloc);
    p->value() = val;
    p->set_mt_safe(); // This value originated from a literal, must be constant
    value_handle result;
    get(result) = p;
    return result;
}

template <impl::allocator A>
auto basic_script_builder_impl<A>::create_value_unsigned(
                                                config::value_unsigned_type val)
    -> value_handle
{
    auto p = basic_value_unsigned<A>::create(alloc);
    p->value() = val;
    p->set_mt_safe(); // This value originated from a literal, must be constant
    value_handle result;
    get(result) = p;
    return result;
}

} // namespace threadscript
