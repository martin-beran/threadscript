/*! \file
 * \brief Tests of error location classes threadscript::src_location,
 * threadscript::frame_location, threadscript::stack_trace and of exception
 * classes declared in namespace threadscript::exception.
 * \todo Add tests for all classes from exception.hpp
 */

//! \cond
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
    std::ostringstream os;
    os << exc;
    BOOST_TEST(os.str() == exc.what());
}
//! \endcond
