#pragma once

/*! \file
 * \brief The implementation part of vm_data.hpp
 */

#include "threadscript/vm_data.hpp"
#include "threadscript/code.hpp"

namespace threadscript {

/*** basic_value ************************************************************/

template <impl::allocator A> typename basic_value<A>::value_ptr
basic_value<A>::arg(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                    const basic_code_node<A>& node, size_t idx)
{
    if (idx >= narg(node))
        return nullptr;
    auto p = node._children[idx];
    assert(p);
    return p->eval(thread, l_vars);
}

template <impl::allocator A> size_t
basic_value<A>::arg_index(basic_state<A>& thread, basic_symbol_table<A>& l_vars,
                          const basic_code_node<A>& node, size_t idx)
{
    auto arg = this->arg(thread, l_vars, node, idx);
    if (!arg)
        throw exception::value_null();
    if (auto pi = dynamic_cast<basic_value_int<A>*>(arg.get())) {
        if (pi->cvalue() < 0)
            throw exception::value_out_of_range();
        return size_t(pi->cvalue());
    } else if (auto pi = dynamic_cast<basic_value_unsigned<A>*>(arg.get()))
        return size_t(pi->cvalue());
    else
        throw exception::value_type();
}

template <impl::allocator A>
auto basic_value<A>::eval(basic_state<A>&, basic_symbol_table<A>&,
    const basic_code_node<A>&, std::string_view) -> value_ptr
{
    return this->shared_from_this();
}

template <impl::allocator A> template <std::derived_from<basic_value<A>> T>
typename basic_value<A>::value_ptr
basic_value<A>::make_result(basic_state<A>& thread,
                            basic_symbol_table<A>& l_vars,
                            const basic_code_node<A>& node,
                            typename T::value_type&& val,
                            bool use_arg, size_t arg)
{
    if (use_arg) {
        auto a0 = this->arg(thread, l_vars, node, arg);
        if (auto pr = dynamic_cast<T*>(a0.get())) {
            pr->value() = std::move(val);
            return a0;
        }
    }
    auto pr = T::create(thread.get_allocator());
    pr->value() = std::move(val);
    return pr;
}

template <impl::allocator A>
size_t basic_value<A>:: narg(const basic_code_node<A>& node) const noexcept
{
    return node._children.size();
}

template <impl::allocator A>
void basic_value<A>::set_mt_safe()
{
    _mt_safe = true;
}

/*** basic_typed_value ******************************************************/

template <class Derived, class T, str_literal Name, impl::allocator A>
basic_typed_value<Derived, T, Name, A>::basic_typed_value(tag, const A& alloc):
    basic_typed_value(tag2{}, alloc)
{
}

//! \cond
template <class Derived, class T, str_literal Name, impl::allocator A>
template <class Alloc> requires impl::uses_allocator<T, Alloc>
basic_typed_value<Derived, T, Name, A>::basic_typed_value(tag2, const Alloc& a):
    data{a}
{
}

template <class Derived, class T, str_literal Name, impl::allocator A>
template <class Alloc> requires (!impl::uses_allocator<T, Alloc>)
basic_typed_value<Derived, T, Name, A>::basic_typed_value(tag2, const Alloc&):
    data{}
{
}
//! \endcond

template <class Derived, class T, str_literal Name, impl::allocator A>
auto basic_typed_value<Derived, T, Name, A>::shallow_copy_impl(const A& alloc,
                                                    std::optional<bool> mt_safe)
    const -> typename basic_value<A>::value_ptr
{
    auto p = create(alloc);
    p->value() = value();
    if (mt_safe.value_or(this->mt_safe()))
        p->set_mt_safe();
    return p;
}

template <class Derived, class T, str_literal Name, impl::allocator A>
std::string_view basic_typed_value<Derived, T, Name, A>::type_name()
    const noexcept
{
    return Name;
}

/*** basic_value_bool ********************************************************/

template <impl::allocator A>
void basic_value_bool<A>::write(std::ostream& os) const
{
    if (this->cvalue())
        os << "true";
    else
        os << "false";
}

/*** basic_value_int *********************************************************/

template <impl::allocator A>
void basic_value_int<A>::write(std::ostream& os) const
{
    os << this->cvalue();
}

/*** basic_value_unsigned ****************************************************/

template <impl::allocator A>
void basic_value_unsigned<A>::write(std::ostream& os) const
{
    os << this->cvalue();
}

/*** basic_value_string ******************************************************/

template <impl::allocator A>
void basic_value_string<A>::write(std::ostream& os) const
{
    os << this->cvalue();
}

/*** basic_value_vector ******************************************************/

template <impl::allocator A> void basic_value_vector<A>::set_mt_safe()
{
    for (auto&& v: this->cvalue())
        if (v && !v->mt_safe())
            throw exception::value_mt_unsafe();
    return impl::basic_value_vector_base<A>::set_mt_safe(); }

/*** basic_value_hash ********************************************************/

template <impl::allocator A> void basic_value_hash<A>::set_mt_safe()
{
    for (auto&& v: this->cvalue())
        if (v.second && !v.second->mt_safe())
            throw exception::value_mt_unsafe();
    return impl::basic_value_hash_base<A>::set_mt_safe();
}

/*** basic_value_object ******************************************************/

template <class Object, str_literal Name, impl::allocator A>
basic_value_object<Object, Name, A>::basic_value_object(tag,
                                  std::shared_ptr<const method_table> methods,
                                  basic_state<A>&, basic_symbol_table<A>&,
                                  const basic_code_node<A>&):
    methods(std::move(methods))
{
    // This static_assert must be at a place where Object is a complete class
    static_assert(std::is_base_of_v<basic_value_object, Object>);
}

template <class Object, str_literal Name, impl::allocator A> auto
basic_value_object<Object, Name, A>::eval(basic_state<A>& thread,
                                          basic_symbol_table<A>& l_vars,
                                          const basic_code_node<A>& node,
                                          std::string_view)
    -> value_ptr
{
    auto narg = this->narg(node);
    if (narg < 1)
        return this->shared_from_this();
    auto a0 = this->arg(thread, l_vars, node, 0);
    if (!a0)
        throw exception::value_null();
    auto method = dynamic_cast<basic_value_string<A>*>(a0.get());
    if (!method)
        throw exception::value_type();
    if (auto m = methods->find(method->cvalue()); m != methods->end())
        return (static_cast<Object*>(this)->*(m->second))(thread, l_vars, node);
    else
        throw exception::not_implemented(method->cvalue());
}

template <class Object, str_literal Name, impl::allocator A>
auto basic_value_object<Object, Name, A>::init_methods() -> method_table
{
    return method_table();
}

template <class Object, str_literal Name, impl::allocator A>
void basic_value_object<Object, Name, A>::register_constructor(
                                                    basic_symbol_table<A>& sym,
                                                    bool replace)
{
    a_basic_string<A> name{std::string_view(Name), sym.get_allocator()};
    if (replace || !sym.contains(name))
        sym.insert(name, constructor::create(sym.get_allocator()));
}

template <class Object, str_literal Name, impl::allocator A> auto
basic_value_object<Object, Name, A>::shallow_copy_impl(const A&,
                                                 std::optional<bool>)
    const -> value_ptr
{
    throw exception::not_implemented("Clone");
}

template <class Object, str_literal Name, impl::allocator A>
std::string_view basic_value_object<Object, Name, A>::type_name() const noexcept
{
    return Name;
}

/*** basic_value_object::constructor *****************************************/

template <class Object, str_literal Name, impl::allocator A>
basic_value_object<Object, Name, A>::constructor::constructor(tag,
                                                              const A& alloc):
    methods(std::allocate_shared<const method_table>(alloc,
                                                     Object::init_methods()))
{
}

template <class Object, str_literal Name, impl::allocator A> auto
basic_value_object<Object, Name, A>::constructor::create(const A& alloc)
    -> value_ptr
{
    return std::allocate_shared<constructor>(alloc, tag{}, alloc);
}

template <class Object, str_literal Name, impl::allocator A> auto
basic_value_object<Object, Name, A>::constructor::eval(basic_state<A>& thread,
                                                 basic_symbol_table<A>& l_vars,
                                                 const basic_code_node<A>& node,
                                                 std::string_view)
    -> value_ptr
{
    return std::allocate_shared<Object>(thread.get_allocator(), tag{}, methods,
                                        thread, l_vars, node);
}

template <class Object, str_literal Name, impl::allocator A> auto
basic_value_object<Object, Name, A>::constructor::shallow_copy_impl(
                                                          const A&,
                                                          std::optional<bool>)
    const -> value_ptr
{
    throw exception::not_implemented("Clone");
}

template <class Object, str_literal Name, impl::allocator A>
std::string_view basic_value_object<Object, Name, A>::constructor::type_name()
    const noexcept
{
            return constructor_type;
}

} // namespace threadscript
