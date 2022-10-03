/*! \file
 * \brief Tests of class threadscript::basic_channel
 */

//! \cond
#include "threadscript/threadscript.hpp"

#define BOOST_TEST_MODULE channel
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include "script_runner.hpp"

#include <thread>

auto sh_vars = test::make_sh_vars<ts::channel>();
//! \endcond

/*! \file
 * \test \c create_object -- Creates a threadscript::basic_channel object */
//! \cond
BOOST_DATA_TEST_CASE(create_object, (std::vector<test::runner_result>{
    {R"(channel())", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(channel(1, 2))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(channel(null))", test::exc{
            typeid(ts::exception::value_null),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Null value"
        }, ""},
    {R"(channel(false))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Bad value type"
        }, ""},
    {R"(channel("123"))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Bad value type"
        }, ""},
    {R"(channel(vector()))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Bad value type"
        }, ""},
    {R"(channel(hash()))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Bad value type"
        }, ""},
    {R"(channel(-1))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Value out of range"
        }, ""},
    {R"(channel(-128))", test::exc{
            typeid(ts::exception::value_out_of_range),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Value out of range"
        }, ""},
    {R"(type(channel(0)))", "channel", ""},
    {R"(type(channel(1)))", "channel", ""},
    {R"(type(channel(2)))", "channel", ""},
    {R"(type(channel(20)))", "channel", ""},
    {R"(type(channel(+100)))", "channel", ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_balance -- Tests method
 * threadscript::basic_channel::balance() */
//! \cond
BOOST_DATA_TEST_CASE(method_balance, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", channel(1)),
            o("balance", 1)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("balance")
        ))", test::int_t(0), ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_recv -- Tests method threadscript::basic_channel::recv() */
//! \cond
BOOST_DATA_TEST_CASE(method_recv, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", channel(1)),
            o("recv", 1)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("send", null),
            o("recv")
        ))", nullptr, ""},
    {R"(seq(
            var("o", channel(1)),
            o("send", true),
            o("recv")
        ))", true, ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_send -- Tests method threadscript::basic_channel::send() */
//! \cond
BOOST_DATA_TEST_CASE(method_send, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", channel(1)),
            o("send")
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("send", 1, 2)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("send", clone(1))
        ))", test::exc{
            typeid(ts::exception::value_mt_unsafe),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Thread-unsafe value"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("send", 1)
        ))", nullptr, ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_try_recv -- Tests method
 * threadscript::basic_channel::try_recv() */
//! \cond
BOOST_DATA_TEST_CASE(method_try_recv, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", channel(1)),
            o("try_recv", 1)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", channel(0)),
            o("try_recv")
        ))", test::exc{
            typeid(ts::exception::op_would_block),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Operation would block"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("try_recv")
        ))", test::exc{
            typeid(ts::exception::op_would_block),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Operation would block"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("send", null),
            o("try_recv")
        ))", nullptr, ""},
    {R"(seq(
            var("o", channel(1)),
            o("send", "MSG"),
            o("try_recv")
        ))", "MSG", ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c method_try_send -- Tests method
 * threadscript::basic_channel::try_send() */
//! \cond
BOOST_DATA_TEST_CASE(method_try_send, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", channel(1)),
            o("try_send")
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("try_send", 1, 2)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", channel(0)),
            o("try_send", clone(1))
        ))", test::exc{
            typeid(ts::exception::value_mt_unsafe),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Thread-unsafe value"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("try_send", clone(1))
        ))", test::exc{
            typeid(ts::exception::value_mt_unsafe),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Thread-unsafe value"
        }, ""},
    {R"(seq(
            var("o", channel(0)),
            o("try_send", "msg")
        ))", test::exc{
            typeid(ts::exception::op_would_block),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Operation would block"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("try_send", "msg"),
            o("try_send", "msg")
        ))", test::exc{
            typeid(ts::exception::op_would_block),
            ts::frame_location("", "", 4, 13),
            "Runtime error: Operation would block"
        }, ""},
    {R"(seq(
            var("o", channel(1)),
            o("try_send", +1)
        ))", nullptr, ""},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c types -- Tests sending and receiving data of various types */
//! \cond
BOOST_DATA_TEST_CASE(types, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", channel(10)),
            o("send", null),
            o("recv")
        ))", nullptr, ""},
    {R"(seq(
            var("o", channel(10)),
            o("send", false),
            o("recv")
        ))", false, ""},
    {R"(seq(
            var("o", channel(10)),
            o("send", 123),
            o("recv")
        ))", test::uint_t(123U), ""},
    {R"(seq(
            var("o", channel(10)),
            o("send", -123),
            o("recv")
        ))", test::int_t(-123), ""},
    {R"(seq(
            var("o", channel(10)),
            o("send", "str"),
            o("recv")
        ))", "str", ""},
    {R"(seq(
            var("o", channel(10)),
            var("v", vector()),
            at(v(), 0, "element"),
            mt_safe(v()),
            o("send", v()),
            var("r", o("recv")),
            print(r(), " ", at(r(), 0))
        ))", nullptr, "vector element"},
    {R"(seq(
            var("o", channel(10)),
            var("v", hash()),
            at(v(), "key", "element"),
            mt_safe(v()),
            o("send", v()),
            var("r", o("recv")),
            print(r(), " ", at(r(), "key"))
        ))", nullptr, "hash element"},
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c methods -- Tests various patterns of sending and receiving data via
 * a channel in a single thread */
//! \cond
BOOST_DATA_TEST_CASE(methods, (std::vector<test::runner_result>{
    // TODO
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond
