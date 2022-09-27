/*! \file
 * \brief Tests of class threadscript::basic_shared_vector
 */

//! \cond
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE shared_vector
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include "script_runner.hpp"

auto sh_vars = test::make_sh_vars<ts::shared_vector>();
//! \endcond

/*! \file
 * \test \c create_object -- Creates a threadscript::basic_shared_vector object
 */
//! \cond
BOOST_DATA_TEST_CASE(create_object, (std::vector<test::runner_result>{
    {R"(type(shared_vector()))", "shared_vector", ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_at -- Tests method threadscript::basic_shared_vector::at()
 */
//! \cond
BOOST_DATA_TEST_CASE(method_at, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", shared_vector()),
            o("at")
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 1, 2, 3)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", null)
        ))", test::exc{
            typeid(ts::exception::value_null),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Null value"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", null, 0)
        ))", test::exc{
            typeid(ts::exception::value_null),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Null value"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, null),
            o("at", 0)
        ))", nullptr, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", false)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", false, 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", "0")
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", "0", 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", -1)
        ))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Value out of range"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", -1, 0)
        ))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Value out of range"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0)
        ))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Value out of range"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, "a"),
            o("at", 0)
        ))", "a", ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, "a"),
            o("at", 1)
        ))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 4, 13),
            "Runtime error: Value out of range"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 2, "X"),
            print(o("at", 0), "\n"),
            print(o("at", 1), "\n"),
            print(o("at", 2), "\n"),
            print(o("at", 3), "\n")
        ))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 7, 19),
            "Runtime error: Value out of range"
        }, "null\nnull\nX\n"},
    {R"(seq(
            var("v", clone(true)),
            var("o", shared_vector()),
            o("at", 0, v())
        ))", test::exc{
            typeid(ts::exception::value_mt_unsafe),
            ts::frame_location("", "", 4, 13),
            "Runtime error: Thread-unsafe value"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, false),
            o("at", 1, 11),
            o("at", 2, -22),
            o("at", 3, "abcd"),
            print(o("at", 0), " ", o("at", 1), " ", o("at", 2), " ",
                o("at", 3), "\n"),
            o("at", 1, mt_safe(add(100, o("at", 1)))),
            print(o("at", 1), "\n"),
            o("at", 5, mt_safe(vector())),
            o("at", 6, mt_safe(hash())),
            print(o("at", 4), " ", o("at", 5), " ", o("at", 6), "\n")
        ))", nullptr,
            "false 11 -22 abcd\n"
            "111\n"
            "null vector hash\n"
    },
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_erase -- Tests method
 * threadscript::basic_shared_vector::erase() */
//! \cond
BOOST_DATA_TEST_CASE(method_erase, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", shared_vector()),
            o("erase", 2, 3)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("erase", null)
        ))", test::exc{
            typeid(ts::exception::value_null),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Null value"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("erase", false)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("erase", "XYZ")
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("erase", vector())
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("erase", hash())
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("erase", -1)
        ))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Value out of range"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("erase"),
            o("size")
        ))", test::uint_t(0U), ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 2, "X"),
            print(o("size"), ":", o("at", 0), ",", o("at", 1), ",",
                o("at", 2), "\n"),
            o("erase"),
            o("size")
        ))", test::uint_t(0U), "3:null,null,X\n"},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, "X"),
            o("at", 1, "Y"),
            o("at", 2, "Z"),
            print(o("size"), ":", o("at", 0), o("at", 1), o("at", 2), "\n"),
            o("erase", 0),
            o("size")
        ))", test::uint_t(0U), "3:XYZ\n"},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, "X"),
            o("at", 1, "Y"),
            o("at", 2, "Z"),
            print(o("size"), ":", o("at", 0), o("at", 1), o("at", 2), "\n"),
            o("erase", 1),
            print(o("at", 0)),
            o("size")
        ))", test::uint_t(1U), "3:XYZ\nX"},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, "X"),
            o("at", 1, "Y"),
            o("at", 2, "Z"),
            print(o("size"), ":", o("at", 0), o("at", 1), o("at", 2), "\n"),
            o("erase", 2),
            print(o("at", 0), o("at", 1)),
            o("size")
        ))", test::uint_t(2U), "3:XYZ\nXY"},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, "X"),
            o("at", 1, "Y"),
            o("at", 2, "Z"),
            print(o("size"), ":", o("at", 0), o("at", 1), o("at", 2), "\n"),
            o("erase", +2),
            print(o("at", 0), o("at", 1)),
            o("size")
        ))", test::uint_t(2U), "3:XYZ\nXY"},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, "X"),
            o("at", 1, "Y"),
            o("at", 2, "Z"),
            print(o("size"), ":", o("at", 0), o("at", 1), o("at", 2), "\n"),
            o("erase", 3),
            print(o("at", 0), o("at", 1), o("at", 2)),
            o("size")
        ))", test::uint_t(3U), "3:XYZ\nXYZ"},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, "X"),
            o("at", 1, "Y"),
            o("at", 2, "Z"),
            print(o("size"), ":", o("at", 0), o("at", 1), o("at", 2), "\n"),
            o("erase", 4),
            print(o("at", 0), o("at", 1), o("at", 2)),
            o("size")
        ))", test::uint_t(3U), "3:XYZ\nXYZ"},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_size -- Tests method
 * threadscript::basic_shared_vector::size() */
//! \cond
BOOST_DATA_TEST_CASE(method_size, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", shared_vector()),
            o("size", 2)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_vector()),
            o("size")
        ))", test::uint_t(0U), ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 0, false),
            o("size")
        ))", test::uint_t(1U), ""},
    {R"(seq(
            var("o", shared_vector()),
            o("at", 20, false),
            o("at", 13, false),
            o("size")
        ))", test::uint_t(21U), ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond
