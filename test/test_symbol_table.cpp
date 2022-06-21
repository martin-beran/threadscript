/*! \file
 * \brief Tests of class threadscript::basic_symbol_table
 */

//! \cond
#include "threadscript/configure.hpp"
#include "threadscript/config_default.hpp"
#include "threadscript/default_allocator.hpp"
#include "threadscript/exception.hpp"
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE symbol_table
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

namespace ts = threadscript;
using namespace std::string_view_literals;

struct tables {
    tables() {
        auto v = ts::value_string::create(alloc);
        v->value() = "a0";
        root.insert("a", v);
        v = ts::value_string::create(alloc);
        v->value() = "a1";
        parent.insert("a", v);
        v = ts::value_string::create(alloc);
        v->value() = "a2";
        child.insert("a", v);
        v = ts::value_string::create(alloc);
        v->value() = "r0";
        root.insert("r", v);
        v = ts::value_string::create(alloc);
        v->value() = "p1";
        parent.insert("p", v);
        v = ts::value_string::create(alloc);
        v->value() = "c2";
        child.insert("c", v);
    }
    ts::allocator_any alloc;
    ts::symbol_table root{alloc, nullptr};
    ts::symbol_table parent{alloc, &root};
    ts::symbol_table child{alloc, &parent};
};

namespace threadscript {

std::ostream& operator<<(std::ostream& os,
                         const std::optional<ts::symbol_table::value_type>& v)
{
    if (v)
        if (*v)
            if (auto s = dynamic_cast<ts::value_string*>(v->get()))
                os << '"' << s->value() << '"';
            else
                os << "bad_type";
        else
            os << "nullptr";
    else
        os << "std::nullopt";
    return os;
}

} // namespace threadscript
//! \endcond

/*! \file
 * \test \c parent -- Symbol tables with and without a parent. */
//! \cond
BOOST_AUTO_TEST_CASE(parent)
{
    tables t;
    BOOST_TEST(t.child.parent_table() == &t.parent);
    BOOST_TEST(t.parent.parent_table() == &t.root);
    BOOST_TEST(t.root.parent_table() == nullptr);
}

//! \endcond

/*! \file
 * \test \c lookup -- Lookup of symbols in a symbol table and its ancestors */
//! \cond
BOOST_AUTO_TEST_CASE(lookup)
{
    tables t;
    auto check = [](const ts::symbol_table& t, const ts::a_string& k,
                    std::optional<ts::a_string> found,
                    std::optional<ts::a_string> parent)
    {
        BOOST_TEST(t.contains(k) == bool(parent));
        BOOST_TEST(t.contains(k, true) == bool(parent));
        BOOST_TEST(t.contains(k, false) == bool(found));
        auto v_default = t.lookup(k);
        auto v_child = t.lookup(k, false);
        auto v_parent = t.lookup(k, true);
        BOOST_TEST(v_default == v_parent);
        if (found) {
            BOOST_TEST(v_child);
            BOOST_TEST(*v_child);
            auto v = dynamic_cast<ts::value_string*>(v_child->get());
            BOOST_TEST(v);
            BOOST_TEST(v->value() == *found);
        } else
            BOOST_TEST(!v_child);
        if (parent) {
            BOOST_TEST(v_parent);
            BOOST_TEST(*v_parent);
            auto v = dynamic_cast<ts::value_string*>(v_parent->get());
            BOOST_TEST(v);
            BOOST_TEST(v->value() == *parent);
        } else
            BOOST_TEST(!v_parent);
    };
    check(t.root, "unknown", std::nullopt, std::nullopt);
    check(t.parent, "unknown", std::nullopt, std::nullopt);
    check(t.child, "unknown", std::nullopt, std::nullopt);
    check(t.root, "a", "a0", "a0");
    check(t.parent, "a", "a1", "a1");
    check(t.child, "a", "a2", "a2");
    check(t.root, "r", "r0", "r0");
    check(t.parent, "r", std::nullopt, "r0");
    check(t.child, "r", std::nullopt, "r0");
    check(t.root, "p", std::nullopt, std::nullopt);
    check(t.parent, "p", "p1", "p1");
    check(t.child, "p", std::nullopt, "p1");
    check(t.root, "c", std::nullopt, std::nullopt);
    check(t.parent, "c", std::nullopt, std::nullopt);
    check(t.child, "c", "c2", "c2");
}
//! \endcond

/*! \file
 * \test \c insert_erase -- Inserting and erasing valus in a symbol table */
//! \cond
BOOST_AUTO_TEST_CASE(insert_erase)
{
    tables t;
    BOOST_TEST(t.child.contains("a", false));
    BOOST_TEST(!t.child.insert("a", nullptr));
    BOOST_TEST(t.child.erase("a"));
    BOOST_TEST(!t.child.erase("a"));
    BOOST_TEST(!t.child.contains("a", false));
    BOOST_TEST(t.child.contains("a", true));
    BOOST_TEST(t.child.insert("a", nullptr));
    BOOST_TEST(t.child.contains("a", false));
}
//! \endcond

/*! \file
 * \test \c storage -- Access the storage container of a symbol table */
//! \cond
BOOST_AUTO_TEST_CASE(storage)
{
    tables t;
    const ts::symbol_table& ctable = t.child;
    static_assert(std::is_same_v<decltype(t.child.symbols()),
                    ts::symbol_table::storage&>);
    static_assert(std::is_same_v<decltype(ctable.symbols()),
                    const ts::symbol_table::storage&>);
    static_assert(std::is_same_v<decltype(t.child.csymbols()),
                    const ts::symbol_table::storage&>);
    BOOST_TEST(&t.child.symbols() == &ctable.symbols());
    BOOST_TEST(&t.child.symbols() == &t.child.csymbols());
    BOOST_TEST(t.child.symbols().size() == 2);
}
//! \endcond
