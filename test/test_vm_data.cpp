/* \file
 * \brief Tests of value types.
 * It tests threadscript::basic_value, threadscript::basic_typed_value, and
 * derived value classes for specific data types:
 * \arg threadscript::basic_value_bool
 * \arg threadscript::basic_value_int
 * \arg threadscript::basic_value_unsigned
 * \arg threadscript::basic_value_string
 */

//! \cond
#include "threadscript/config.hpp"
#include "threadscript/config_default.hpp"
#include "threadscript/default_allocator.hpp"
#include "threadscript/exception.hpp"
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE vm_data
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

namespace ts = threadscript;
using namespace std::string_view_literals;

using value_types = std::tuple<
    ts::value_bool,
    ts::value_int,
    ts::value_unsigned,
    ts::value_string
>;

template <class T> struct properties {};
template <> struct properties<ts::value_bool> {
    static constexpr std::string_view type_name{"bool"};
    inline static const bool default_value{};
    inline static const bool set_value{true};
};
template <> struct properties<ts::value_int> {
    static constexpr std::string_view type_name{"int"};
    inline static const ts::config::value_int_type default_value{};
    inline static const ts::config::value_int_type set_value{-123};
};
template <> struct properties<ts::value_unsigned> {
    static constexpr std::string_view type_name{"unsigned"};
    inline static const ts::config::value_unsigned_type default_value{};
    inline static const ts::config::value_unsigned_type set_value{234};
};
template <> struct properties<ts::value_string> {
    static constexpr std::string_view type_name{"string"};
    inline static const ts::a_string default_value{};
    inline static const ts::a_string set_value{"abc"};
};
//! \endcond

/*! \file
 * \test \c value_default -- Default-constructed value of a type derived from
 * threadscript::basic_typed_value */
//! \cond
BOOST_AUTO_TEST_CASE_TEMPLATE(value_default, T, value_types)
{
    ts::allocator_any alloc;
    auto v = T::create(alloc);
    BOOST_TEST(T::static_type_name() == properties<T>::type_name);
    BOOST_TEST(v->type_name() == properties<T>::type_name);
    BOOST_TEST(v->value() == properties<T>::default_value);
}
//! \endcond

/*! \file
 * \test \c value_set -- Setting a value of a type derived from
 * threadscript::basic_typed_value */
//! \cond
BOOST_AUTO_TEST_CASE_TEMPLATE(value_set, T, value_types)
{
    ts::allocator_any alloc;
    auto v = T::create(alloc);
    static_assert(std::is_same_v<decltype(v), typename T::typed_value_ptr>);
    BOOST_REQUIRE(properties<T>::default_value != properties<T>::set_value);
    v->value() = properties<T>::set_value;
    BOOST_TEST(v->value() == properties<T>::set_value);
    BOOST_TEST(v->cvalue() == properties<T>::set_value);
    BOOST_TEST(typename T::const_typed_value_ptr(v)->value() ==
               properties<T>::set_value);
}
//! \endcond

/*! \file
 * \test \c value_shallow_copy -- Copying a value of a type derived from
 * threadscript::basic_typed_value */
//! \cond
BOOST_AUTO_TEST_CASE_TEMPLATE(value_shallow_copy, T, value_types)
{
    ts::allocator_any alloc;
    ts::value::value_ptr untyped = T::create(alloc);
    dynamic_pointer_cast<T>(untyped)->value() = properties<T>::set_value;
    auto untyped_copy = untyped->shallow_copy(alloc);
    static_assert(std::is_same_v<decltype(untyped_copy), ts::value::value_ptr>);
    BOOST_TEST(dynamic_pointer_cast<T>(untyped_copy)->value() ==
               properties<T>::set_value);
    auto typed = T::create(alloc);
    typed->value() = properties<T>::set_value;
    static_assert(std::is_same_v<decltype(typed), typename T::typed_value_ptr>);
    auto typed_copy = typed->shallow_copy(alloc);
    static_assert(std::is_same_v<decltype(typed), decltype(typed_copy)>);
    BOOST_TEST(typed_copy->value() == properties<T>::set_value);
}
//! \endcond

/*! \file
 * \test \c value_mt_safe -- Handling thread-safety flag of a type derived
 * from threadscript::basic_typed_value */
//! \cond
BOOST_AUTO_TEST_CASE_TEMPLATE(value_mt_safe, T, value_types)
{
    ts::allocator_any alloc;
    auto v = T::create(alloc);
    BOOST_TEST(!v->mt_safe());
    v->value() = properties<T>::set_value;
    v->set_mt_safe();
    BOOST_TEST(v->mt_safe());
    BOOST_TEST(v->cvalue() == properties<T>::set_value);
    BOOST_TEST(typename T::const_typed_value_ptr(v)->value() ==
               properties<T>::set_value);
    BOOST_CHECK_THROW(v->value(), ts::exception::value_mt_unsafe);
}
//! \endcond

/*! \file
 * \test \c value_string_allocator -- The internal value of
 * threadscript::basic_value_string uses the provided allocator. */
//! \cond
BOOST_AUTO_TEST_CASE(value_string_allocator)
{
    ts::allocator_config cfg;
    ts::allocator_any alloc{&cfg};
    auto v = ts::value_string::create(alloc);
    BOOST_TEST(alloc.cfg() == v->value().get_allocator().cfg());
}
//! \endcond

/*! \file
 * \test \c value_string_capacity -- Automatic handling of
 * threadscript::basic_value_string capacity */
//! \cond
BOOST_AUTO_TEST_CASE(value_string_capacity)
{
    ts::allocator_any alloc;
    const size_t c_empty = std::string{}.capacity();
    auto v = ts::value_string::create(alloc);
    BOOST_TEST(v->value().capacity() == c_empty);
    v->value().append(100, 'A');
    BOOST_TEST(v->value().capacity() >= v->value().size());
    v->value().resize(20);
    BOOST_TEST(v->value().capacity() < 40);
    v->value().clear();
    v->value().append(100, 'A');
    size_t c0 = v->value().capacity();
    BOOST_TEST(c0 >= v->value().size());
    std::vector<size_t> cap;
    for (auto s: {c0 - 1, c0 - 25, c0 / 3 + 1, c0 / 3}) {
        v->value().resize(s);
        cap.push_back(v->value().capacity());
    }
    for (size_t i = 0; i < cap.size() - 1; ++i)
        BOOST_TEST(cap[i] == c0);
    BOOST_TEST(v->value().size() == cap.back());
}
//! \endcond

/*! \file
 * \test \c value_array_default -- Default-constructed value of
 * threadscript::basic_value_array */
//! \cond
BOOST_AUTO_TEST_CASE(value_array_default)
{
    ts::allocator_any alloc;
    auto v = ts::value_array::create(alloc);
    BOOST_TEST(ts::value_array::static_type_name() == "array"sv);
    BOOST_TEST(v->type_name() == "array"sv);
    BOOST_TEST(v->value().empty());
}
//! \endcond

/*! \file
 * \test \c value_array_set -- Setting a value of
 * threadscript::basic_value_array */
//! \cond
BOOST_AUTO_TEST_CASE(value_array_set)
{
    ts::allocator_any alloc;
    auto v = ts::value_array::create(alloc);
    v->value().push_back(ts::value_int::create(alloc));
    BOOST_TEST(v->value()[0]->type_name() == "int"sv);
    BOOST_TEST(v->cvalue()[0]->type_name() == "int"sv);
    BOOST_TEST(ts::value_array::const_typed_value_ptr(v)->value()[0]->
               type_name() == "int"sv);
}
//! \endcond

/*! \file
 * \test value_array_shallow_copy -- Copying a value of
 * threadscript::basic_value_array */
//! \cond
BOOST_AUTO_TEST_CASE(value_array_shallow_copy)
{
    ts::allocator_any alloc;
    ts::value::value_ptr untyped = ts::value_array::create(alloc);
    auto a = dynamic_pointer_cast<ts::value_array>(untyped);
    size_t n = 10;
    for (size_t i = 0; i < n; ++i) {
        auto v = ts::value_unsigned::create(alloc);
        v->value() = i;
        a->value(). push_back(v);
    }
    auto untyped_copy = untyped->shallow_copy(alloc);
    static_assert(std::is_same_v<decltype(untyped_copy), ts::value::value_ptr>);
    auto ac = dynamic_pointer_cast<ts::value_array>(untyped_copy);
    BOOST_TEST(a->value().size() == n);
    BOOST_TEST(ac->value().size() == n);
    for (size_t i = 0; i < n; ++i)
        BOOST_TEST(a->value()[i].get() == ac->value()[i].get());
    auto typed = ts::value_array::create(alloc);
    for (size_t i = 0; i < n; ++i) {
        auto v = ts::value_unsigned::create(alloc);
        v->value() = i;
        typed->value().push_back(v);
    }
    static_assert(std::is_same_v<decltype(typed),
                  ts::value_array::typed_value_ptr>);
    auto typed_copy = typed->shallow_copy(alloc);
    static_assert(std::is_same_v<decltype(typed_copy),
                  ts::value_array::typed_value_ptr>);
    BOOST_TEST(typed->value().size() == n);
    BOOST_TEST(typed_copy->value().size() == n);
    for (size_t i = 0; i < n; ++i)
        BOOST_TEST(typed->value().size() == typed_copy->value().size());
}
//! \endcond

/*! \file
 * \test value_array_mt_safe -- Handling thread-safety flag of
 * threadscript::value_array */
//! \cond
BOOST_AUTO_TEST_CASE(value_array_mt_safe)
{
    ts::allocator_any alloc;
    auto v = ts::value_array::create(alloc);
    BOOST_TEST(!v->mt_safe());
    v->value().push_back(ts::value_int::create(alloc));
    v->set_mt_safe();
    BOOST_TEST(v->mt_safe());
    BOOST_TEST(v->cvalue().size() == 1);
    BOOST_TEST(ts::value_array::const_typed_value_ptr(v)->value().size() == 1);
    BOOST_CHECK_THROW(v->value(), ts::exception::value_mt_unsafe);
}
//! \endcond

/*! \file
 * \test \c value_array_allocator -- The internal value of
 * threadscript::basic_value_array uses the provided allocator. */
//! \cond
BOOST_AUTO_TEST_CASE(value_array_allocator)
{
    ts::allocator_config cfg;
    ts::allocator_any alloc{&cfg};
    auto v = ts::value_array::create(alloc);
    BOOST_TEST(alloc.cfg() == v->value().get_allocator().cfg());
}
//! \endcond

/*! \file
 * \test value_array_capacity -- Automatic handling of
 * threadscript::basic_value_array capacity */
//! \cond
BOOST_AUTO_TEST_CASE(value_array_capacity)
{
    ts::allocator_any alloc;
    const size_t c_empty = std::vector<ts::value::value_ptr>{}.capacity();
    auto v = ts::value_array::create(alloc);
    BOOST_TEST(v->value().capacity() == c_empty);
    v->value().resize(100);
    BOOST_TEST(v->value().capacity() >= v->value().size());
    v->value().resize(20);
    BOOST_TEST(v->value().capacity() < 40);
    v->value().clear();
    v->value().resize(100);
    size_t c0 = v->value().capacity();
    BOOST_TEST(c0 >= v->value().size());
    std::vector<size_t> cap;
    for (auto s: {c0 - 1, c0 - 25, c0 / 3 + 1, c0 / 3}) {
        v->value().resize(s);
        cap.push_back(v->value().capacity());
    }
    for (size_t i = 0; i < cap.size() - 1; ++i)
        BOOST_TEST(cap[i] == c0);
    BOOST_TEST(v->value().size() == cap.back());
}
//! \endcond
