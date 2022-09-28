/*! \file
 * \brief Tests of class threadscript::basic_shared_hash
 */

//! \cond
#include "threadscript/threadscript.hpp"
#include "threadscript/symbol_table_impl.hpp"

#define BOOST_TEST_MODULE shared_hash
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include "script_runner.hpp"

#include <thread>

auto sh_vars = test::make_sh_vars<ts::shared_hash>();
//! \endcond

/*! \file
 * \test \c create_object -- Creates a threadscript::basic_shared_hash object */
//! \cond
BOOST_DATA_TEST_CASE(create_object, (std::vector<test::runner_result>{
    {R"(type(shared_hash()))", "shared_hash", ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_at -- Tests method threadscript::basic_shared_hash::at()
 */
//! \cond
BOOST_DATA_TEST_CASE(method_at, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", shared_hash()),
            o("at")
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "1", 2, 3)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", null)
        ))", test::exc{
            typeid(ts::exception::value_null),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Null value"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", null, 0)
        ))", test::exc{
            typeid(ts::exception::value_null),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Null value"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "key", null),
            o("at", "key")
        ))", nullptr, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", false)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", false, 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", 1, 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", -2)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", -2, 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", vector())
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", vector(), 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", hash())
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", hash(), 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "key")
        ))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Value out of range"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "key", "a"),
            o("at", "key")
        ))", "a", ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "key", "a"),
            o("at", "unknown")
        ))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 4, 13),
            "Runtime error: Value out of range"
        }, ""},
    {R"(seq(
            var("v", clone(true)),
            var("o", shared_hash()),
            o("at", "key", v())
        ))", test::exc{
            typeid(ts::exception::value_mt_unsafe),
            ts::frame_location("", "", 4, 13),
            "Runtime error: Thread-unsafe value"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "k0", false),
            o("at", "k1", 11),
            o("at", "k2", -22),
            o("at", "k3", "abcd"),
            print(o("at", "k0"), " ", o("at", "k1"), " ", o("at", "k2"), " ",
                o("at", "k3"), "\n"),
            o("at", "k1", mt_safe(add(100, o("at", "k1")))),
            print(o("at", "k1"), "\n"),
            o("at", "k5", mt_safe(vector())),
            o("at", "k6", mt_safe(hash())),
            print(o("at", "k5"), " ", o("at", "k6"), "\n")
        ))", nullptr,
        "false 11 -22 abcd\n"
        "111\n"
        "vector hash\n"
    },
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_erase -- Tests method
 * threadscript::basic_shared_hash::erase() */
//! \cond
BOOST_DATA_TEST_CASE(method_erase, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", shared_hash()),
            o("erase", "2", 3)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("erase", null)
        ))", test::exc{
            typeid(ts::exception::value_null),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Null value"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("erase", false)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("erase", 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("erase", -3)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("erase", vector())
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("erase", hash())
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("erase"),
            o("size")
        ))", test::uint_t(0U), ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "k0", "X"),
            o("at", "k1", "Y"),
            o("at", "k2", "Z"),
            print(o("size"), ":", o("at", "k0"), o("at", "k1"),
                o("at", "k2"), "\n"),
            o("erase"),
            o("size")
        ))", test::uint_t(0U), "3:XYZ\n"},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "k0", "X"),
            o("at", "k1", "Y"),
            o("at", "k2", "Z"),
            print(o("size"), ":", o("at", "k0"), o("at", "k1"),
                o("at", "k2"), "\n"),
            o("erase", "k0"),
            o("erase", "k1"),
            o("erase", "k2"),
            o("size")
        ))", test::uint_t(0U), "3:XYZ\n"},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "k0", "X"),
            o("at", "k1", "Y"),
            o("at", "k2", "Z"),
            print(o("size"), ":", o("at", "k0"), o("at", "k1"),
                o("at", "k2"), "\n"),
            o("erase", "k1"),
            o("erase", "k2"),
            print(o("at", "k0")),
            o("size")
        ))", test::uint_t(1U), "3:XYZ\nX"},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "k0", "X"),
            o("at", "k1", "Y"),
            o("at", "k2", "Z"),
            print(o("size"), ":", o("at", "k0"), o("at", "k1"),
                o("at", "k2"), "\n"),
            o("erase", "k2"),
            print(o("at", "k0"), o("at", "k1")),
            o("size")
        ))", test::uint_t(2U), "3:XYZ\nXY"},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "k0", "X"),
            o("at", "k1", "Y"),
            o("at", "k2", "Z"),
            print(o("size"), ":", o("at", "k0"), o("at", "k1"),
                o("at", "k2"), "\n"),
            o("erase", "k3"),
            print(o("at", "k0"), o("at", "k1"), o("at", "k2")),
            o("size")
        ))", test::uint_t(3U), "3:XYZ\nXYZ"},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_keys -- Tests method
 * threadscript::basic_shared_hash::keys() */
//! \cond
BOOST_DATA_TEST_CASE(method_keys, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", shared_hash()),
            o("keys", 2)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            var("k", o("keys")),
            print(type(k())),
            size(k())
        ))", test::uint_t(0U), "vector"},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "Xy", 0),
            o("at", "xyz", 1),
            o("at", "", 2),
            o("at", "a", 3),
            o("at", "bc", 4),
            var("k", o("keys")),
            print(at(k(), 0), ",", at(k(), 1), ",", at(k(), 2), ",",
                at(k(), 3), ",", at(k(), 4)),
            size(k())
        ))", test::uint_t(5U), ",Xy,a,bc,xyz"},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "Xy", 0),
            o("at", "xyz", 1),
            var("k", o("keys")),
            print(is_mt_safe(at(k(), 0)), ",", is_mt_safe(at(k(), 1))),
            size(k())
        ))", test::uint_t(2U), "true,true"},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_size -- Tests method
 * threadscript::basic_shared_hash::size() */
//! \cond
BOOST_DATA_TEST_CASE(method_size, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", shared_hash()),
            o("size", 2)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("size")
        ))", test::uint_t(0U), ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "k0", false),
            o("size")
        ))", test::uint_t(1U), ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "AB", false),
            o("at", "BC", false),
            o("at", "CD", false),
            o("at", "BC", true),
            o("at", "DEF", false),
            o("size")
        ))", test::uint_t(4U), ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond
