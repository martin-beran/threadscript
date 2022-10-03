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
    {R"(shared_hash(1))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 1, 1),
            "Runtime error: Bad number of arguments"
        }, ""},
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
 * \test \c method_contains -- Tests method
 * threadscript::basic_shared_hash::contains() */
//! \cond
BOOST_DATA_TEST_CASE(method_contains, (std::vector<test::runner_result>{
    {R"(seq(
            var("o", shared_hash()),
            o("contains")
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("contains", "2", 3)
        ))", test::exc{
            typeid(ts::exception::op_narg),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad number of arguments"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("contains", null)
        ))", test::exc{
            typeid(ts::exception::value_null),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Null value"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("contains", false)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("contains", 1)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("contains", -2)
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("contains", vector())
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("contains", hash())
        ))", test::exc{
            typeid(ts::exception::value_type),
            ts::frame_location("", "", 3, 13),
            "Runtime error: Bad value type"
        }, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("contains", "key")
        ))", false, ""},
    {R"(seq(
            var("o", shared_hash()),
            o("at", "A", 1),
            o("at", "B", 2),
            print(o("contains", "A"), ",", o("contains", "B"), ",",
                o("contains", "C"))
        ))", nullptr, "true,true,false"},
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

/*! \file
 * \test \c threads -- Tests accessing threadscript::basic_shared_hash from
 * multiple threads */
//! \cond
BOOST_AUTO_TEST_CASE(threads)
{
    auto script = R"(
seq(
    gvar("counters", shared_hash()),
    gvar("e", shared_hash()),
    gvar("ei", shared_hash()),
    counters("at", "", 0),
    gvar("key", vector()),
    at(key(), 0, ""),
    at(key(), 1, "A"), at(key(), 2, "B"), at(key(), 3, "C"), at(key(), 4, "D"),
    at(key(), 5, "E"), at(key(), 6, "F"), at(key(), 7, "G"), at(key(), 8, "H"),
    at(key(), 9, "I"), at(key(), 10, "J"),
    mt_safe(key()),
    gvar("num_threads", size(key())),
    sub(num_threads(), num_threads(), 1),
    mt_safe(num_threads()),

    fun("f_main", seq(
        var("iter", clone(0)),
        var("tmp_b0", clone(false)),
        var("tmp_b1", clone(false)),
        var("tmp_u", clone(0)),
        while(lt(tmp_b0(), iter(), max()), seq(
            var("i", clone(1)),
            var("step", true),
            while(le(tmp_b0(), i(), num_threads()), seq(
                var("k", at(key(), i())),
                if(
                    or_r(
                        tmp_b0(),
                        not(tmp_b1(), counters("contains", k())),
                        ne(tmp_b1(), counters("at", k()), counters("at", ""))
                    ),
                    var("step", false)
                ),
                add(i(), i(), 1)
            )),
            if(step(), seq(
                counters("at", "", mt_safe(add(counters("at", ""), 1))),
                if(or_r(
                    tmp_b0(),
                    lt(tmp_b1(), e("size"), num_threads()),
                    lt(tmp_b1(), ei("size"), num_threads())
                ), seq(
                    var("i", clone(1)),
                    while(le(tmp_b0(), i(), num_threads()), seq(
                        e("at", at(key(), i()), mt_safe(clone(iter()))),
                        ei("at", at(key(), i()), mt_safe(clone(iter()))),
                        add(i(), i(), 1)
                    ))
                )),
                add(iter(), iter(), 1)
            ))
        )),
        var("ok", true),
        var("i", clone(0)),
        while(lt(i(), counters("size")), seq(
            var("ci", counters("at", at(key(), i()))),
            if(and(
                ne(ci(), max()),
                ne(ci(), sub(max(), 1))
            ), seq(
                var("ok", false),
                print("i=", i(), " val=", ci(), " max=", max(), "\n")
            )),
            add(i(), i(), 1)
        )),
        print("ok=", ok()),
        ok()
    )),

    fun("f_thread", seq(
        var("t_idx", at(_args(), 0)),
        var("k", at(key(), t_idx())),
        var("run", true),
        var("tmp_b", clone(false)),
        while(run(), seq(
            if(not(tmp_b(), counters("contains", k())),
                counters("at", k(), 0)
            ),
            if(gt(counters("at", ""), counters("at", k())), seq(
                counters("at", k(), mt_safe(clone(counters("at", "")))),
                e("erase"),
                ei("erase", k())
            )),
            if(eq(counters("at", k()), max()),
                var("run", false)
            )
        ))
    ))
)
    )"sv;
    // Prepare VM
    constexpr unsigned max = 200;
    std::ostringstream std_out;
    ts::virtual_machine vm{test::alloc};
    vm.std_out = &std_out;
    // Pass variables from C++ to the script
    auto sh_vars = test::make_sh_vars<ts::shared_hash>();
    auto set_uint = [&sh_vars](auto&& name, auto val) {
        auto v = ts::value_unsigned::create(sh_vars->get_allocator());
        v->value() = val;
        v->set_mt_safe();
        sh_vars->insert(name, v);
    };
    set_uint("max", max);
    vm.sh_vars = sh_vars;
    // Parse and run the script, defining functions and variables and storing
    // them in the global shared symbol table
    auto parsed = ts::parse_code(vm.get_allocator(), script, "string");
    {
        ts::state s_prepare{vm};
        BOOST_REQUIRE_NO_THROW(
            try {
                parsed->eval(s_prepare);
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
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
    auto v_num_threads =
        dynamic_pointer_cast<ts::value_unsigned>(sh_vars->lookup("num_threads").
                                                 value_or(nullptr));
    BOOST_REQUIRE_NE(v_num_threads, nullptr);
    const unsigned num_threads = v_num_threads->cvalue();
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
    BOOST_REQUIRE_NO_THROW(
        try {
            result = dynamic_pointer_cast<ts::value_bool>(f_main->call(s_main,
                                                                   fname_main));
        } catch (std::exception& e) {
            BOOST_TEST_INFO("exception: " << e.what());
            throw;
        });
    // Wait for threads and check the result
    for (auto&& t: threads)
        t.join();
    BOOST_REQUIRE_NE(result, nullptr);
    BOOST_CHECK(result->cvalue());
    BOOST_CHECK_EQUAL(std_out.view(), "ok=true"sv);
}
//! \endcond
