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
    inline static const std::string default_value{};
    inline static const std::string set_value{"abc"};
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
 * \test \c value_shallow_copy -- Handling thread-safety flag of a type derived
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
