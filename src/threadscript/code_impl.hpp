#pragma once

/*! \file
 * \brief The implementation part of code.hpp
 */

#include "threadscript/code.hpp"

namespace threadscript {

/*** basic_code_node *********************************************************/

template <impl::allocator A>
basic_code_node<A>::basic_code_node(tag, const A& alloc,
                                    const file_location& location,
                                    std::string_view name,
                                    value_t value):
    location(location), name(name, alloc), _children(alloc), value(value)
{
}

template <impl::allocator A> basic_code_node<A>::~basic_code_node() {
    typename std::allocator_traits<A>::template rebind_alloc<basic_code_node<A>>
        alloc{_children.get_allocator()};
    using traits = std::allocator_traits<decltype(alloc)>;
    for (priv_ptr c: _children)
        if (c) {
            traits::destroy(alloc, c);
            alloc.deallocate(c, 1);
        }
}

template <impl::allocator A>
bool basic_code_node<A>::operator==(const basic_code_node& o) const noexcept
{
    if (location != o.location || name != o.name)
        return false;
    if (bool(value) != bool(o.value))
        return false;
    if (bool(value) && bool(o.value) && bool(*value) != bool(*o.value))
            return false;
    if (_children.size() != o._children.size())
        return false;
    for (auto [a, b] = std::pair{_children.begin(), o._children.begin()};
         a != _children.end();
         ++a, ++b)
    {
        assert(b != o._children.end());
        if (*a && *b) {
            if (**a != **b)
                return false;
        } else
            if (bool(*a) != bool(*b))
                return false;
    }
    return true;
}

template <impl::allocator A>
void basic_code_node<A>::write(std::ostream& os, size_t indent) const
{
    std::string i_string(indent, ' ');
    os << indent << name << '@' << location << '[';
    if (value)
        os << value->get();
    else
        os << "nullopt";
    os << "](\n";
    for (auto c: _children)
        c->write(os, indent + indent_step);
    os << indent << '\n';
}

template <impl::allocator A>
std::ostream& operator<<(std::ostream& os, const basic_code_node<A>& node)
{
    node.write(os, 0);
    return os;
}

/*** basic_script ************************************************************/

template <impl::allocator A>
basic_script<A>::basic_script(tag, const A& alloc, std::string_view file):
    _file(file, alloc), alloc(alloc)
{
}
    
template <impl::allocator A> basic_script<A>::~basic_script() {
    if (_root) {
        typename std::allocator_traits<A>::template rebind_alloc<node_type>
            a{alloc};
        using traits = std::allocator_traits<decltype(a)>;
        traits::destroy(a, _root);
        a.deallocate(_root, 1);
    }
}

template <impl::allocator A>
auto basic_script<A>::add_node(const node_ptr& parent,
                                   const file_location& location,
                                   std::string_view name,
                                   typename node_type::value_t value) ->
    node_ptr
{
    if (!parent && _root)
        throw exception::parse_error("Root node already exists");
    typename node_type::priv_ptr p = nullptr;
    bool constructed = false;
    typename std::allocator_traits<A>::template rebind_alloc<node_type>
        a{alloc};
    using traits = std::allocator_traits<decltype(a)>;
    try {
        p = a.allocate(1);
        traits::construct(a, p, typename node_type::tag{}, alloc, location,
                          name, value);
        constructed = true;
        if (parent) {
            // OK during building script tree
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            const_cast<node_type&>(*parent)._children.push_back(p);
        } else
            _root = p;
    } catch (...) {
        if (constructed)
            traits::destroy(a, p);
        if (p)
            a.deallocate(p, 1);
        throw;
    }
    return node_ptr{this->shared_from_this(), p};
}

template <impl::allocator A>
bool basic_script<A>::operator==(const basic_script& o) const noexcept
{
    if (_file != o._file)
        return false;
    if (_root && o._root)
        return *_root == *o._root;
    return !_root && !o._root;
}

template <impl::allocator A>
std::ostream& operator<<(std::ostream& os, const basic_script<A>& script)
{
    os << script.file() << " {\n";
    if (script._root)
        script._root->write(os, basic_script<A>::node_type::indent_step);
    os << "}\n";
    return os;
}

} // namespace threadscript
