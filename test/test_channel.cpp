/*! \file
 * \brief Tests of class threadscript::basic_channel
 */

//! \cond
#include "threadscript/threadscript.hpp"
#include "threadscript/symbol_table_impl.hpp"

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
    {R"(seq(
            var("o", channel(3)),
            o("send", "a"),
            print(o("recv"), "\n"),
            o("send", "b"), o("send", "c"), o("send", "d"),
            print(o("recv"), o("recv"), o("recv"), "\n"),
            o("send", "e"), o("send", "f"),
            print(o("recv"), "\n"),
            o("send", "g"), o("send", "h"),
            print(o("recv"), "\n"),
            o("try_send", "i"),
            try(
                o("try_send", "j"),
                "op_would_block", seq(
                    print(o("recv"), "\n"),
                    o("try_send", "J")
                )
            ),
            print(o("recv"), o("recv"), o("recv"), "\n")
        ))", nullptr,
            "a\n"
            "bcd\n"
            "e\n"
            "f\n"
            "g\n"
            "hiJ\n"
        },
    {R"(seq(
            var("o", channel(3)),
            o("try_send", "a"),
            print(o("try_recv"), "\n"),
            o("try_send", "b"), o("try_send", "c"), o("try_send", "d"),
            print(o("try_recv"), o("try_recv"), o("try_recv"), "\n"),
            o("try_send", "e"), o("try_send", "f"),
            print(o("try_recv"), "\n"),
            o("try_send", "g"), o("try_send", "h"),
            print(o("try_recv"), "\n"),
            o("try_send", "i"),
            try(
                o("try_send", "j"),
                "op_would_block", seq(
                    print(o("try_recv"), "\n"),
                    o("try_send", "J")
                )
            ),
            print(o("try_recv"), o("try_recv"), o("try_recv"), "\n")
        ))", nullptr,
            "a\n"
            "bcd\n"
            "e\n"
            "f\n"
            "g\n"
            "hiJ\n"
        },
    {R"(seq(
            var("o", channel(3)),
            o("send", "a"),
            print(o("try_recv"), "\n"),
            o("try_send", "b"),
            print(o("recv"), "\n"),
            o("send", "c"), o("try_send", "d"),
            print(o("recv"), o("try_recv"), "\n"),
            o("try_send", "e"), o("send", "f"),
            print(o("try_recv"), o("recv"), "\n")
        ))", nullptr,
            "a\n"
            "b\n"
            "cd\n"
            "ef\n"
        },
}))
{
    test::check_runner(sample, sh_vars);
}
//! \endcond

/*! \file
 * \test \c threads -- Tests various patterns of sending and receiving data via
 * a channel in multiple threads */
//! \cond
BOOST_DATA_TEST_CASE(threads, (std::vector<test::runner_result>{
    {R"(seq(
            # capacity 1, send + recv
            gvar("num_threads", 1),
            gvar("o", channel(1)),
            fun("f_main", seq(
                o("recv")
            )),
            fun("f_thread", seq(
                o("send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 1, multiple send + recv
            gvar("num_threads", 1),
            gvar("o", channel(1)),
            fun("f_main", seq(
                print(o("recv"), o("recv"), o("recv"))
            )),
            fun("f_thread", seq(
                o("send", "MSG1"),
                o("send", "MSG2"),
                o("send", "MSG3")
            ))
        ))", nullptr, "MSG1MSG2MSG3"},
    {R"(seq(
            # capacity 1, send + try_recv
            gvar("num_threads", 1),
            gvar("o", channel(1)),
            gvar("ctrl", channel(1)),
            fun("f_main", seq(
                var("v", null),
                var("start", true),
                while(is_null(v()), try(
                    var("v", o("try_recv")),
                    "op_would_block", if(start(), seq(
                        ctrl("send", null),
                        var("start", false)
                    ))
                )),
                v()
            )),
            fun("f_thread", seq(
                ctrl("recv"), # at least one failed o("try_recv")
                o("send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 1, try_send + recv
            gvar("num_threads", 1),
            gvar("o", channel(1)),
            gvar("ctrl", channel(1)),
            fun("f_main", seq(
                ctrl("recv"), # at least one failed o("try_send")
                print(o("recv")),
                o("recv")
            )),
            fun("f_thread", seq(
                o("send", "MSG1"),
                var("wait", true),
                var("start", true),
                while(wait(), try(
                    seq(
                        o("try_send", "MSG2"),
                        var("wait", false)
                    ),
                    "op_would_block", if(start(), seq(
                        ctrl("send", null),
                        var("start", false)
                    ))
                ))
            ))
        ))", "MSG2", "MSG1"},
    {R"(seq(
            # capacity 1, try_send + try_recv
            gvar("num_threads", 1),
            gvar("o", channel(1)),
            fun("f_main", seq(
                var("run", true),
                while(run(), try(
                    seq(
                        var("v", o("try_recv")),
                        print(v()),
                        var("run", not(is_null(v())))
                    ),
                    "op_would_block", seq()
                ))
            )),
            fun("f_thread", seq(
                var("i", 0),
                while(lt(i(), 10), try(
                    seq(
                        o("try_send", i()),
                        var("i", mt_safe(add(i(), 1)))
                    ),
                    "op_would_block", seq()
                )),
                o("send", null)
            ))
        ))", false, "0123456789null"},
    {R"(seq(
            # capacity 0, send + recv
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            fun("f_main", seq(
                o("recv")
            )),
            fun("f_thread", seq(
                o("send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 1, multiple send + recv
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            fun("f_main", seq(
                print(o("recv"), o("recv"), o("recv"))
            )),
            fun("f_thread", seq(
                o("send", "MSG1"),
                o("send", "MSG2"),
                o("send", "MSG3")
            ))
        ))", nullptr, "MSG1MSG2MSG3"},
    {R"(seq(
            # capacity 0, send + recv, wait for blocked send
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            fun("f_main", seq(
                while(ne(o("balance"), +1), o("balance")),
                o("recv")
            )),
            fun("f_thread", seq(
                o("send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 0, send + recv, wait for blocked recv
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            fun("f_main", seq(
                o("recv")
            )),
            fun("f_thread", seq(
                while(ne(o("balance"), -1), seq()),
                o("send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 0, send + try_recv
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            gvar("ctrl", channel(1)),
            fun("f_main", seq(
                var("v", null),
                var("start", true),
                while(is_null(v()), try(
                    var("v", o("try_recv")),
                    "op_would_block", if(start(), seq(
                        ctrl("send", null),
                        var("start", false)
                    ))
                )),
                v()
            )),
            fun("f_thread", seq(
                ctrl("recv"), # at least one failed o("try_recv")
                o("send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 0, send + try_recv, wait for blocked send
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            fun("f_main", seq(
                while(ne(o("balance"), +1), o("balance")),
                o("try_recv")
            )),
            fun("f_thread", seq(
                o("send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 0, send + try_recv, wait for multiple blocked senders
            gvar("num_threads", 2),
            gvar("o", channel(0)),
            fun("f_main", seq(
                while(ne(o("balance"), +2), o("balance")),
                print(o("try_recv"), o("try_recv"))
            )),
            fun("f_thread", seq(
                o("send", "MSG")
            ))
        ))", nullptr, "MSGMSG"},
    {R"(seq(
            # capacity 0, try_send + recv
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            gvar("ctrl", channel(1)),
            fun("f_main", seq(
                ctrl("recv"), # at least one failed o("try_send")
                o("recv")
            )),
            fun("f_thread", seq(
                var("wait", true),
                var("start", true),
                while(wait(), try(
                    seq(
                        o("try_send", "MSG"),
                        var("wait", false)
                    ),
                    "op_would_block", if(start(), seq(
                        ctrl("send", null),
                        var("start", false)
                    ))
                ))
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 0, try_send + recv, wait for blocked recv
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            fun("f_main", seq(
                o("recv")
            )),
            fun("f_thread", seq(
                while(ne(o("balance"), -1), seq()),
                o("try_send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 0, try_send + recv, wait for multiple blocked receivers
            gvar("num_threads", 5),
            gvar("o", channel(0)),
            fun("f_main", seq(
                o("recv")
            )),
            fun("f_thread", seq(
                if(gt(at(_args(), 0), 0), seq(
                    o("recv")
                ), seq(
                    while(ne(o("balance"), -5), seq()),
                    o("try_send", "MSG"),
                    o("try_send", "MSG"),
                    o("try_send", "MSG"),
                    o("try_send", "MSG"),
                    o("try_send", "MSG")
                ))
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity 0, try_send + try_recv
            gvar("num_threads", 1),
            gvar("o", channel(0)),
            gvar("fail", channel(1)),
            fun("f_main", seq(
                var("i", clone(0)),
                var("err", clone(0)),
                while(lt(i(), 10000), seq(
                    try(seq(
                        o("try_recv"),
                        add(err(), err(), 1)
                    ), "op_would_block", seq(
                    )),
                    add(i(), i(), 1)
                )),
                print("try_send=", fail("recv"), " try_recv=", err())
            )),
            fun("f_thread", seq(
                var("i", clone(0)),
                var("err", clone(0)),
                while(lt(i(), 10000), seq(
                    try(seq(
                        o("try_send", null),
                        add(err(), err(), 1)
                    ), "op_would_block", seq(
                    )),
                    add(i(), i(), 1)
                )),
                mt_safe(err()),
                fail("send", err())
            ))
        ))", nullptr, "try_send=0 try_recv=0"},
    {R"(seq(
            # capacity >1, send + recv
            gvar("num_threads", 1),
            gvar("o", channel(3)),
            fun("f_main", seq(
                o("recv")
            )),
            fun("f_thread", seq(
                o("send", "MSG")
            ))
        ))", "MSG", ""},
    {R"(seq(
            # capacity >1, multiple send + recv
            gvar("num_threads", 1),
            gvar("o", channel(3)),
            fun("f_main", seq(
                print(o("recv"), o("recv"), o("recv"), o("recv"), o("recv"))
            )),
            fun("f_thread", seq(
                o("send", "MSG1"),
                o("send", "MSG2"),
                o("send", "MSG3"),
                o("send", "MSG4"),
                o("send", "MSG5")
            ))
        ))", nullptr, "MSG1MSG2MSG3MSG4MSG5"},
    {R"(seq(
            # capacity >1, send + try_recv
            gvar("num_threads", 1),
            gvar("o", channel(3)),
            gvar("ctrl", channel(1)),
            fun("f_main", seq(
                var("v", clone("")),
                var("n", clone(0)),
                var("start", true),
                while(lt(n(), 5), try(
                    seq(
                        add(v(), v(), o("try_recv")),
                        add(n(), n(), 1)
                    ),
                    "op_would_block", if(start(), seq(
                        ctrl("send", null),
                        var("start", false)
                    ))
                )),
                v()
            )),
            fun("f_thread", seq(
                ctrl("recv"), # at least one failed o("try_recv")
                o("send", "1"),
                o("send", "2"),
                o("send", "3"),
                o("send", "4"),
                o("send", "5")
            ))
        ))", "12345", ""},
    {R"(seq(
            # capacity >1, try_send + recv
            gvar("num_threads", 1),
            gvar("o", channel(3)),
            gvar("ctrl", channel(1)),
            fun("f_main", seq(
                ctrl("recv"), # at least one failed o("try_send")
                print(o("recv"), o("recv"), o("recv")),
                print(o("recv"), o("recv"), o("recv"), o("recv"), o("recv"))
            )),
            fun("f_thread", seq(
                o("send", "MSG1"),
                o("send", "MSG2"),
                o("send", "MSG3"),
                var("wait", true),
                var("start", true),
                var("n", clone(0)),
                while(lt(n(), 5), try(
                    seq(
                        o("try_send", mt_safe(clone(n()))),
                        add(n(), n(), 1)
                    ),
                    "op_would_block", if(start(), seq(
                        ctrl("send", null),
                        var("start", false)
                    ))
                ))
            ))
        ))", nullptr, "MSG1MSG2MSG301234"},
    {R"(seq(
            # capacity >1, try_send + try_recv
            gvar("num_threads", 1),
            gvar("o", channel(4)),
            fun("f_main", seq(
                var("run", true),
                while(run(), try(
                    seq(
                        var("v", o("try_recv")),
                        print(v()),
                        var("run", not(is_null(v())))
                    ),
                    "op_would_block", seq()
                ))
            )),
            fun("f_thread", seq(
                var("i", 0),
                while(lt(i(), 10), try(
                    seq(
                        o("try_send", i()),
                        var("i", mt_safe(add(i(), 1)))
                    ),
                    "op_would_block", seq()
                )),
                o("send", null)
            ))
        ))", false, "0123456789null"},
    {R"(seq(
            # 1-to-N
            gvar("num_threads", 10),
            gvar("o", channel(2)),
            gvar("done", channel(20)),
            gvar("num_vals", 1000),
            fun("f_main", seq(
                var("i", clone(0)),
                while(lt(i(), mul(num_threads(), num_vals())), seq(
                    o("send", mt_safe(clone(i()))),
                    add(i(), i(), 1)
                )),
                var("ok", true),
                var("i", clone(0)),
                while(lt(i(), num_threads()), seq(
                    if(ne(done("recv"), num_vals()),
                        var("ok", false)
                    ),
                    add(i(), i(), 1)
                )),
                ok()
            )),
            fun("f_thread", seq(
                var("i", clone(0)),
                while(lt(i(), num_vals()), seq(
                    o("recv"),
                    add(i(), i(), 1)
                )),
                done("send", mt_safe(i()))
            ))
        ))", true, ""},
    {R"(seq(
            # M-to-1
            gvar("num_threads", 10),
            gvar("o", channel(15)),
            gvar("num_vals", 1000),
            fun("f_main", seq(
                var("i", clone(0)),
                while(lt(i(), mul(num_threads(), num_vals())), seq(
                    o("recv"),
                    add(i(), i(), 1)
                ))
            )),
            fun("f_thread", seq(
                var("i", clone(0)),
                while(lt(i(), num_vals()), seq(
                    o("send", mt_safe(clone(i()))),
                    add(i(), i(), 1)
                ))
            ))
        ))", test::uint_t(10000U), ""},
    {R"(seq(
            # M-to-N
            gvar("mn", 10),
            gvar("num_threads", mul(2, mn())),
            gvar("o", channel(4)),
            gvar("num_vals", 1000),
            fun("f_main", seq(
            )),
            fun("f_thread", seq(
                var("i", clone(0)),
                while(lt(i(), num_vals()), seq(
                    if(lt(at(_args(), 0), mn()),
                        o("send", mt_safe(clone(i()))),
                        o("recv")
                    ),
                    add(i(), i(), 1)
                ))
            ))
        ))", nullptr, ""},
}))
{
    test::check_runner<test::script_runner_threads>(sample, sh_vars);
}
//! \endcond
