/*! \file
 * \brief Tests of class threadscript::syntax::canon
 */

//! \cond
#include "threadscript/syntax.hpp"

#define BOOST_TEST_MODULE parser
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace ts = threadscript;

/*! \file
 * \test \c syntax_names -- Check the list of support syntax variants */
//! \cond
BOOST_AUTO_TEST_CASE(syntax_names)
{
    std::map<std::string, bool> expect{{"canon", false}};
    for (auto&& n: ts::syntax_factory::names()) {
        auto it = expect.find(std::string(n));
        BOOST_TEST_INFO("UNEXPECTED " << n);
        BOOST_CHECK(it != expect.end());
        if (it != expect.end())
            it->second = true;
    }
    for (auto&& n: expect) {
        BOOST_TEST_INFO("MISSING " << n.first);
        BOOST_CHECK(n.second);
    }
}
//! \endcond
