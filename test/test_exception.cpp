/*! \file
 * \brief Tests of error location classes threadscript::src_location,
 * threadscript::frame_location, threadscript::stack_trace and of exception
 * classes declared in namespace threadscript::exception.
 */

//! \cond
#include "threadscript/debug.hpp"
#include "threadscript/exception.hpp"
#include <sstream>

#define BOOST_TEST_MODULE allocator_config
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
//! \endcond

namespace ts = threadscript;
namespace ex = ts::exception;

/*! \file
 * \test \c src_location_none -- Using threadscript::src_location without file,
 * line, and column */
//! \cond
BOOST_AUTO_TEST_CASE(src_location_none)
{
    ts::src_location loc;
    BOOST_TEST(loc.file.empty());
    BOOST_TEST(loc.line == loc.unknown);
    BOOST_TEST(loc.column == loc.unknown);
    BOOST_TEST(loc.to_string() == "::");
    std::ostringstream os;
    os << loc;
    BOOST_TEST(os.str() == loc.to_string());
}
//! \endcond

/*! \file
 * \test \c src_location -- Using threadscript::src_location with file, line,
 * and column */
//! \cond
BOOST_AUTO_TEST_CASE(src_location)
{
    ts::src_location loc("script.ts", 5, 12);
    BOOST_TEST(loc.file == "script.ts");
    BOOST_TEST(loc.line == 5);
    BOOST_TEST(loc.column == 12);
    BOOST_TEST(loc.to_string() == "script.ts:5:12");
    std::ostringstream os;
    os <<loc;
    BOOST_TEST(os.str() == loc.to_string());
}
//! \endcond

/*! \file
 * \test \c frame_location_none -- Using threadscript::frame_location without
 * function, file, line, column */
//! \cond
BOOST_AUTO_TEST_CASE(frame_location_none)
{
    ts::frame_location loc("", "");
    BOOST_TEST(loc.function.empty());
    BOOST_TEST(loc.file.empty());
    BOOST_TEST(loc.line == loc.unknown);
    BOOST_TEST(loc.column == loc.unknown);
    BOOST_TEST(loc.to_string() == ":::()");
    std::ostringstream os;
    os << loc;
    BOOST_TEST(os.str() == loc.to_string());
}
//! \endcond

/*! \file
 * \test \c frame_location -- Using threadscript::frame_location with function,
 * file, line, column */
//! \cond
BOOST_AUTO_TEST_CASE(frame_location)
{
    ts::frame_location loc("test_fun", "library.tsl", 210, 34);
    BOOST_TEST(loc.function == "test_fun");
    BOOST_TEST(loc.file == "library.tsl");
    BOOST_TEST(loc.line == 210);
    BOOST_TEST(loc.column == 34);
    BOOST_TEST(loc.to_string() == "library.tsl:210:34:test_fun()");
    std::ostringstream os;
    os << loc;
    BOOST_TEST(os.str() == loc.to_string());
}
//! \endcond

/*! \file
 * \test \c stack_trace_empty -- An empty threadscript::stack_trace */
//! \cond
BOOST_AUTO_TEST_CASE(stack_trace_empty)
{
    ts::stack_trace trace;
    BOOST_TEST(trace.empty());
    auto loc = std::move(trace).location();
    BOOST_TEST(loc.function.empty());
    BOOST_TEST(loc.file.empty());
    BOOST_TEST(loc.line == loc.unknown);
    BOOST_TEST(loc.column == loc.unknown);
    BOOST_TEST(loc.to_string() == ":::()");
    BOOST_TEST(trace.to_string().empty());
    BOOST_TEST(trace.to_string(true).empty());
    BOOST_TEST(trace.to_string(false).empty());
    {
        std::ostringstream os;
        os << trace;
        BOOST_TEST(os.str().empty());
    }
    {
        std::ostringstream os;
        os << trace;
        BOOST_TEST(os.str().empty());
    }
}
//! \endcond

/*! \file
 * \test \c stack_trace -- An nonempty threadscript::stack_trace with multiple
 * frames */
//! \cond
BOOST_AUTO_TEST_CASE(stack_trace)
{
    ts::stack_trace trace;
    trace.emplace_back("main", "script_a", 5, 1);
    trace.emplace_back("func1", "script_b", ts::frame_location::unknown, 11);
    trace.emplace_back("func2", "script_c", 25, ts::frame_location::unknown);
    trace.emplace_back("native", "");
    BOOST_TEST(trace.size() == 4);
    auto loc = trace.location();
    BOOST_TEST(loc.function == "main");
    BOOST_TEST(loc.file == "script_a");
    BOOST_TEST(loc.line == 5);
    BOOST_TEST(loc.column == 1);
    BOOST_TEST(trace.to_string() == trace.to_string(true));
    BOOST_TEST(trace.to_string(true) ==
R"(    0. script_a:5:1:main()
    1. script_b::11:func1()
    2. script_c:25::func2()
    3. :::native()
)");
    BOOST_TEST(trace.to_string(false) == "    0. script_a:5:1:main()\n");
    {
        std::ostringstream os;
        os << ts::stack_trace::full << trace;
        BOOST_TEST(os.str() == trace.to_string(true));
    }
    {
        std::ostringstream os;
        os << trace;
        BOOST_TEST(os.str() == trace.to_string(false));
    }
}
//! \endcond

/*! \file
 * \test \c base_default -- Class threadscript::exception::base with the
 * default message and no stack trace */
//! \cond
BOOST_AUTO_TEST_CASE(base_default)
{
    ex::base exc;
    BOOST_TEST(exc.what() == "ThreadScript exception");
    BOOST_TEST(exc.msg() == "ThreadScript exception");
    BOOST_TEST(exc.trace().empty());
    BOOST_TEST(exc.location().file.empty());
    BOOST_TEST(exc.location().function.empty());
    BOOST_TEST(exc.location().line == ts::frame_location::unknown);
    BOOST_TEST(exc.location().column == ts::frame_location::unknown);
    BOOST_TEST(exc.to_string() == std::string{exc.what()}.append("\n"));
    BOOST_TEST(exc.to_string(true) == exc.to_string());
    BOOST_TEST(exc.to_string(false) == exc.what());
    {
        std::ostringstream os;
        os << exc;
        BOOST_TEST(os.str() == exc.to_string(false));
    }
    {
        std::ostringstream os;
        os << ts::stack_trace::full << exc;
        BOOST_TEST(os.str() == exc.to_string(true));
    }
    ex::base exc_copy = exc;
    BOOST_TEST(&exc != &exc_copy);
    BOOST_TEST(exc_copy.what() == "ThreadScript exception");
    BOOST_TEST(exc_copy.msg() == "ThreadScript exception");
    ex::base exc_move = std::move(exc);
    BOOST_TEST(&exc != &exc_move);
    BOOST_TEST(exc_move.what() == "ThreadScript exception");
    BOOST_TEST(exc_move.msg() == "ThreadScript exception");
    ex::base exc2;
    BOOST_TEST(&exc2 != &exc);
    BOOST_TEST(exc2.what() == "ThreadScript exception");
    exc_copy = exc2;
    BOOST_TEST(exc_copy.what() == "ThreadScript exception");
    BOOST_TEST(exc_copy.msg() == "ThreadScript exception");
    exc_move = std::move(exc2);
    BOOST_TEST(exc_move.what() == "ThreadScript exception");
    BOOST_TEST(exc_move.msg() == "ThreadScript exception");
}
//! \endcond

/*! \file
 * \test \c base_trace -- Class threadscript::exception::base with the default
 * message and a stack trace */
//! \cond
BOOST_AUTO_TEST_CASE(base_trace)
{
    ex::base exc{ts::stack_trace{
        {"main", "script", 10, 1},
        {"fun1", "lib1", 20, 2},
        {"f2", "script2", 30, 3},
    }};
    BOOST_TEST(exc.what() == "script:10:1:main(): ThreadScript exception");
    BOOST_TEST(exc.msg() == "ThreadScript exception");
    BOOST_TEST(exc.trace().size() == 3);
    BOOST_TEST(exc.location().file == "script");
    BOOST_TEST(exc.location().function == "main");
    BOOST_TEST(exc.location().line == 10);
    BOOST_TEST(exc.location().column == 1);
    BOOST_TEST(exc.to_string() ==
R"(script:10:1:main(): ThreadScript exception
    0. script:10:1:main()
    1. lib1:20:2:fun1()
    2. script2:30:3:f2()
)");
    BOOST_TEST(exc.to_string(true) == exc.to_string());
    BOOST_TEST(exc.to_string(false) == exc.what());
    {
        std::ostringstream os;
        os << exc;
        BOOST_TEST(os.str() == exc.to_string(false));
    }
    {
        std::ostringstream os;
        os << ts::stack_trace::full << exc;
        BOOST_TEST(os.str() == exc.to_string(true));
    }
    ex::base exc_copy = exc;
    BOOST_TEST(&exc != &exc_copy);
    BOOST_TEST(exc_copy.what() == "script:10:1:main(): ThreadScript exception");
    BOOST_TEST(exc_copy.msg() == "ThreadScript exception");
    ex::base exc_move = std::move(exc);
    BOOST_TEST(&exc != &exc_move);
    BOOST_TEST(exc_move.what() == "script:10:1:main(): ThreadScript exception");
    BOOST_TEST(exc_move.msg() == "ThreadScript exception");
    ex::base exc2{ts::stack_trace{{"main", "script", 10, 1}}};
    BOOST_TEST(&exc2 != &exc);
    BOOST_TEST(exc2.what() == "script:10:1:main(): ThreadScript exception");
    exc_copy = exc2;
    BOOST_TEST(exc_copy.what() == "script:10:1:main(): ThreadScript exception");
    BOOST_TEST(exc_copy.msg() == "ThreadScript exception");
    exc_move = std::move(exc2);
    BOOST_TEST(exc_move.what() == "script:10:1:main(): ThreadScript exception");
    BOOST_TEST(exc_move.msg() == "ThreadScript exception");
}
//! \endcond

/*! \file
 * \test \c base -- Class threadscript::exception::base with a custom message
 * and a stack trace */
//! \cond
BOOST_AUTO_TEST_CASE(base)
{
    ex::base exc{"Test error message", ts::stack_trace{
        {"main", "script", 10, 1},
        {"fun1", "lib1", 20, 2},
        {"f2", "script2", 30, 3},
    }};
    BOOST_TEST(exc.what() == "script:10:1:main(): Test error message");
    BOOST_TEST(exc.msg() == "Test error message");
    BOOST_TEST(exc.trace().size() == 3);
    BOOST_TEST(exc.location().file == "script");
    BOOST_TEST(exc.location().function == "main");
    BOOST_TEST(exc.location().line == 10);
    BOOST_TEST(exc.location().column == 1);
    BOOST_TEST(exc.to_string() ==
R"(script:10:1:main(): Test error message
    0. script:10:1:main()
    1. lib1:20:2:fun1()
    2. script2:30:3:f2()
)");
    BOOST_TEST(exc.to_string(true) == exc.to_string());
    BOOST_TEST(exc.to_string(false) == exc.what());
    {
        std::ostringstream os;
        os << exc;
        BOOST_TEST(os.str() == exc.to_string(false));
    }
    {
        std::ostringstream os;
        os << ts::stack_trace::full << exc;
        BOOST_TEST(os.str() == exc.to_string(true));
    }
}
//! \endcond

/*! \file
 * \test \c wrapped -- Class threadscript::exception::wrapped */
//! \cond
BOOST_AUTO_TEST_CASE(wrapped)
{
    std::string wrapped_msg;
    std::string inner_msg;
    try {
        try {
            try {
                throw std::runtime_error("Inner runtime error");
            } catch (...) {
                throw(ex::wrapped());
            }
        } catch (ex::wrapped& e) {
            wrapped_msg = e.msg();
            e.rethrow();
        }
    } catch (std::runtime_error& e) {
        inner_msg = e.what();
    }
    BOOST_TEST(wrapped_msg == "ThreadScript wrapped exception");
    BOOST_TEST(inner_msg == "Inner runtime error");
}
//! \endcond

/*! \file
 * \test \c wrapped_msg_trace -- Class threadscript::exception::wrapped with a
 * custom message and a stack trace */
//! \cond
BOOST_AUTO_TEST_CASE(wrapped_msg_trace)
{
    std::string wrapped_msg;
    std::string inner_msg;
    try {
        try {
            try {
                throw std::runtime_error("Inner runtime error");
            } catch (...) {
                throw(ex::wrapped("Custom wrapped msg",
                                  ts::stack_trace{{"main", "script", 10, 1}}));
            }
        } catch (ex::wrapped& e) {
            wrapped_msg = e.msg();
            BOOST_TEST(e.trace().size() == 1);
            e.rethrow();
        }
    } catch (std::runtime_error& e) {
        inner_msg = e.what();
    }
    BOOST_TEST(wrapped_msg == "Custom wrapped msg");
    BOOST_TEST(inner_msg == "Inner runtime error");
}
//! \endcond

/*! \file
 * \test 'c wrapped_exc -- Class threadscript::exception::wrapped with passing
 * a message via an exception object. */
//! \cond
BOOST_AUTO_TEST_CASE(wrapped_exc)
{
    std::string wrapped_msg;
    std::string inner_msg;
    try {
        try {
            try {
                throw std::runtime_error("Inner runtime error");
            } catch (const std::exception& e) {
                throw(ex::wrapped(e,
                                  ts::stack_trace{{"main", "script", 10, 1}}));
            }
        } catch (ex::wrapped& e) {
            wrapped_msg = e.msg();
            BOOST_TEST(e.trace().size() == 1);
            e.rethrow();
        }
    } catch (std::runtime_error& e) {
        inner_msg = e.what();
    }
    BOOST_TEST(wrapped_msg == inner_msg);
    BOOST_TEST(inner_msg == "Inner runtime error");
}
//! \endcond

/*! \file
 * \test \c parse_error -- Class threadscript::exception::parse_error */
//! \cond
BOOST_AUTO_TEST_CASE(parse_error)
{
    ex::parse_error exc("Invalid operator", {{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Parse error: Invalid operator");
}
//! \endcond

/*! \file
 * \test \c alloc_bad -- Class threadscript::exception::alloc_bad */
//! \cond
BOOST_AUTO_TEST_CASE(alloc_bad)
{
    ex::alloc_bad exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Allocation failed");
}
//! \endcond

/*! \file
 * \test \c alloc_limit -- Class threadscript::exception::alloc_limit */
//! \cond
BOOST_AUTO_TEST_CASE(alloc_limit)
{
    ex::alloc_limit exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Allocation denied by limit");
}
//! \endcond

/*! \file
 * \test \c unknown_symbol -- Class threadscript::exception::unknown_symbol */
//! \cond
BOOST_AUTO_TEST_CASE(unknown_symbol)
{
    ex::unknown_symbol exc("var1", {{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Symbol not found: var1");
}
//! \endcond

/*! \file
 * \test \c value_bad -- Class threadscript::exception::value_bad */
//! \cond
BOOST_AUTO_TEST_CASE(value_bad)
{
    ex::value_bad exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Bad value");
}
//! \endcond

/*! \file
 * \test \c value_null -- Class threadscript::exception::value_null */
//! \cond
BOOST_AUTO_TEST_CASE(value_null)
{
    ex::value_null exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Null value");
}
//! \endcond

/*! \file
 * \test \c value_read_only -- Class threadscript::exception::value_read_only
 */
//! \cond
BOOST_AUTO_TEST_CASE(value_read_only)
{
    ex::value_read_only exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Read-only value");
}
//! \endcond

/*! \file
 * \test \c value_type -- Class threadscript::exception::value_type */
//! \cond
BOOST_AUTO_TEST_CASE(value_type)
{
    ex::value_type exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Bad value type");
}
//! \endcond

/*! \file
 * \test \c value_out_of_range -- Class
 * threadscript::exception::value_out_of_range */
//! \cond
BOOST_AUTO_TEST_CASE(value_out_of_range)
{
    ex::value_out_of_range exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Value out of range");
}
//! \endcond

/*! \file
 * \test \c op_bad -- Class threadscript::exception::op_bad */
//! \cond
BOOST_AUTO_TEST_CASE(op_bad)
{
    ex::op_bad exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Bad operation");
}
//! \endcond

/*! \file
 * \test \c op_recursion -- Class threadscript::exception::op_recursion */
//! \cond
BOOST_AUTO_TEST_CASE(op_recursion)
{
    ex::op_recursion exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Recursion too deep");
}
//! \endcond

/*! \file
 * \test \c op_overflow -- Class threadscript::exception::op_overflow */
//! \cond
BOOST_AUTO_TEST_CASE(op_overflow)
{
    ex::op_overflow exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Overflow");
}
//! \endcond

/*! \file
 * \test \c op_div_zero -- Class threadscript::exception::op_div_zero */
//! \cond
BOOST_AUTO_TEST_CASE(op_div_zero)
{
    ex::op_div_zero exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Division by zero");
}
//! \endcond

/*! \file
 * \test \c op_library -- Class threadscript::exception::op_library */
//! \cond
BOOST_AUTO_TEST_CASE(op_library)
{
    ex::op_library exc({{"main", "script", 10, 1}});
    BOOST_TEST(exc.trace().size() == 1);
    BOOST_TEST(exc.to_string(false) ==
               "script:10:1:main(): Runtime error: Library failure");
}
//! \endcond
