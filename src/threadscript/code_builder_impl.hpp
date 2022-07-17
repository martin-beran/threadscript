#pragma

/*! \file
 * \brief Implementation of threadscript::script_builder
 */

#include "threadscript/code_builder.hpp"

namespace threadscript {

template <impl::allocator A> basic_script_builder_impl: public script_builder {
public:
    void create_script(std::string_view file) override;
    node_handle add_node(const node_handle& parent,
                         const file_location& location,
                         std::string_view name,
                         const value_handle& value = value_handle{}) override;
    value_handle create_value_null() override;
    value_handle create_value_bool(bool val) override;
    value_handle create_value_int(config::value_int_type val) override;
    value_handle create_value_unsigned(config::value_unsigned_type val)
        override;
    value_handle create_value_string(std::string_view val) override;
};

/*** basic_script_builder_impl ***********************************************/

template <impl::allocator A>
void basic_script_builder_impl<A>::create_script(std::string_view file)
{
    // TODO
}

template <impl::allocator A>
void basic_script_builder_impl<A>::node_handle add_node(
                                                const node_handle& parent,
                                                const file_location& location,
                                                std::string_view name,
                                                const value_handle& value)
{
    // TODO
}

template <impl::allocator A> value_handle
basic_script_builder_impl<A>::create_value_null()
{
    // TODO
}

template <impl::allocator A> value_handle
basic_script_builder_impl<A>::create_value_bool(bool val)
{
    // TODO
}

template <impl::allocator A> value_handle
basic_script_builder_impl<A>::create_value_int(config::value_int val)
{
    // TODO
}

template <impl::allocator A> value_handle
basic_script_builder_impl<A>::create_value_unsigned(config::value_unsigned val)
{
    // TODO
}

template <impl::allocator A> value_handle
basic_script_builder_impl<A>::create_value_string(config::value_string val)
{
    // TODO
}

} // namespace threadscript
