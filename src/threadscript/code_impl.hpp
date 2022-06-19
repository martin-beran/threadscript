#pragma once

/*! \file
 * \brief The implementation part of code.hpp
 */

#include "threadscript/code.hpp"
#include "threadscript/exception.hpp"
#include "threadscript/finally.hpp"

namespace threadscript {

/*** basic_code_node *********************************************************/

template <impl::allocator A>
basic_code_node<A>::basic_code_node(tag, const A& alloc,
                                    const a_basic_string<A>& file,
                                    const file_location& location,
                                    std::string_view name,
                                    value_t value):
    _file(file), location(location), name(name, alloc), _children(alloc),
    value(value)
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

template <impl::allocator A> typename basic_value<A>::value_ptr
basic_code_node<A>::eval(basic_state<A>& thread,
        const basic_symbol_table<A>& lookup,
        const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& sym)
{
    // Called from a script or function, therefore the stack must be nonempty.
    assert(!thread.stack.empty());
    // We do not call another script or function here, therefore only line and
    // column can change in the location at the top of the stack. Saving just
    // these values and not file and function names prevents throwing and
    // eliminates string copying. Object slicing is OK here.
    finally restore{
        [&thread, loc = file_location{thread.stack.back().location}]() noexcept
        {
            thread.stack.back().location = loc;
        }
    };
    static_cast<file_location&>(thread.stack.back().location) = location;
    // value ? value : lookup.lookup(name) would make a copy of value
    // Initialization of the reference extends lifetime of the temporary object
    // returned by lookup().
    const value_t& v = value ? value :
        static_cast<const value_t&>(lookup.lookup(name));
    if (!v)
        throw exception::unknown_symbol(name, thread.current_stack());
    if (!*v)
        return nullptr;
    return (*v)->eval(thread, lookup, sym, *this);
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
        traits::construct(a, p, typename node_type::tag{}, alloc, _file,
                          location, name, value);
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

template <impl::allocator A> typename basic_value<A>::value_ptr
basic_script<A>::eval(basic_state<A>& thread,
        const basic_symbol_table<A>& lookup,
        const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& sym)
{
    if (_root) {
        auto& frame =
            thread.push_frame(basic_state<A>::stack_frame(alloc, thread));
        finally pop{[&thread]() noexcept { thread.pop_frame(); }};
        frame.file = _file;
        return _root->eval(thread, lookup, sym);
    }
    return nullptr;
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

/*** basic_value_function ****************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
basic_value_function<A>::eval(basic_state<A>& /*thread*/,
        const basic_symbol_table<A>& /*lookup*/,
        const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& /*sym*/,
        const basic_code_node<A>& /*node*/)
{
    // TODO
    return nullptr;
}

/*** basic_value_script ******************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
basic_value_script<A>::eval(basic_state<A>& thread,
        const basic_symbol_table<A>& lookup,
        const std::vector<std::reference_wrapper<basic_symbol_table<A>>>& sym,
        const basic_code_node<A>&)
{
    if (auto script = this->cvalue())
        return script->eval(thread, lookup, sym);
    return nullptr;
}

/*** basic_value_native_fun **************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
basic_value_native_fun<A>::eval(basic_state<A>&,
        const basic_symbol_table<A>&,
        const std::vector<std::reference_wrapper<basic_symbol_table<A>>>&,
        const basic_code_node<A>&)
{
    return nullptr;
}

} // namespace threadscript
