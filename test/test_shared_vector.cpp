/*! \file
 * \brief Tests of class threadscript::basic_shared_vector
 */

//! \cond
#include "threadscript/threadscript.hpp"
#include "threadscript/symbol_table_impl.hpp"

#define BOOST_TEST_MODULE shared_vector
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include "script_runner.hpp"

#include <thread>

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

/*! \file
 * \test \c threads -- Tests accessing threadscript::basic_shared_vector from
 * multiple threads */
//! \cond
BOOST_AUTO_TEST_CASE(threads)
{
    auto script = R"(
seq(
    gvar("counters", shared_vector()),
    gvar("e", shared_vector()),
    gvar("ei", shared_vector()),
    counters("at", 0, 0),

    fun("f_main", seq(
        var("iter", 0),
        while(lt(iter(), max()), seq(
            var("i", 1),
            var("step", true),
            while(le(i(), num_threads()), seq(
                if(
                    or(
                        ge(i(), counters("size")),
                        is_null(counters("at", i())),
                        ne(counters("at", i()), counters("at", 0))
                    ),
                    var("step", false)
                ),
                var("i", add(i(), 1))
            )),
            if(step(), seq(
                counters("at", 0, mt_safe(add(counters("at", 0), 1))),
                if(lt(e("size"), num_threads()),
                    e("at", sub(num_threads(), 1), mt_safe(clone(iter())))
                ),
                if(lt(ei("size"), num_threads()),
                    e("at", sub(num_threads(), 1), mt_safe(clone(iter())))
                ),
                var("iter", add(iter(), 1))
            ))
        )),
        var("ok", true),
        var("i", 0),
        while(lt(i(), counters("size")),
            if(ne(counters("at", i()), max()),
                var("ok", false)
            )
        ),
        print("ok=", ok()),
        ok()
    )),

    fun("f_thread", seq(
        var("t_idx", at(_args(), 0)),
        var("run", true),
        while(run(), seq(
            if(or(
                ge(t_idx(), counters("size")),
                is_null(counters("at", t_idx()))
            ),
                counters("at", t_idx(), 0)
            ),
            if(gt(counters("at", 0), counters("at", t_idx())), seq(
                counters("at", t_idx(), mt_safe(clone(counters("at", 0)))),
                e("erase"),
                ei("erase", t_idx())
            )),
            if(eq(counters("at", t_idx()), max()),
                var("run", false)
            )
        ))
    ))
)
    )"sv;
    //! [run_threads]
    // Prepare VM
    constexpr unsigned num_threads = 10;
    constexpr unsigned max = 100;
    std::ostringstream std_out;
    ts::virtual_machine vm{test::alloc};
    vm.std_out = &std_out;
    // Pass variables from C++ to the script
    auto sh_vars = test::make_sh_vars<ts::shared_vector>();
    auto set_uint = [&sh_vars](auto&& name, auto val) {
        auto v = ts::value_unsigned::create(sh_vars->get_allocator());
        v->value() = val;
        v->set_mt_safe();
        sh_vars->insert(name, v);
    };
    set_uint("num_threads", num_threads);
    set_uint("max", max);
    vm.sh_vars = sh_vars;
    // Parse and run the script, defining functions and variables and storing
    // them in the global shared symbol table
    auto parsed = ts::parse_code(vm.get_allocator(), script, "string");
    {
        ts::state s_prepare{vm};
        BOOST_REQUIRE_NO_THROW(parsed->eval(s_prepare));
        for (auto&& sym: s_prepare.t_vars.csymbols())
            sh_vars->insert(sym.first, sym.second);
    }
    // Run functions in threads
    ts::a_string fname_thread("f_thread", vm.get_allocator());
    auto f_thread =
        dynamic_pointer_cast<ts::value_function>(sh_vars->lookup(fname_thread).
                                                 value_or(nullptr));
    BOOST_REQUIRE_NE(f_thread, nullptr);
    std::vector<std::thread> threads;
    for (size_t t = 0; t < num_threads; ++t)
        threads.emplace_back([&vm, &fname_thread, &f_thread, i = t + 1]() {
            auto args = ts::value_vector::create(vm.get_allocator());
            auto arg_i = ts::value_unsigned::create(vm.get_allocator());
            arg_i->value() = i;
            args->value().push_back(arg_i);
            ts::state s_thread{vm};
            try {
                f_thread->call(s_thread, fname_thread, args);
            } catch (std::exception& e) {
                std::cout << "thread=" << i << " exception=" << e.what() <<
                    std::endl;
            }
        });
    ts::a_string fname_main("f_main", vm.get_allocator());
    auto f_main =
        dynamic_pointer_cast<ts::value_function>(sh_vars->lookup(fname_main).
                                                 value_or(nullptr));
    BOOST_REQUIRE_NE(f_main, nullptr);
    ts::state s_main{vm};
    ts::value_bool::typed_value_ptr result{};
    try {
        result = dynamic_pointer_cast<ts::value_bool>(f_main->call(s_main,
                                                                   fname_main));
    } catch (std::exception& e) {
        std::cout << "main exception=" << e.what() << std::endl;
    }
    // Wait for threads and check the result
    for (auto&& t: threads)
        t.join();
    BOOST_REQUIRE_NE(result, nullptr);
    BOOST_CHECK(result->cvalue());
    BOOST_CHECK_EQUAL(std_out.view(), "ok=true"sv);
    //! [run_threads]
}
//! \endcond
