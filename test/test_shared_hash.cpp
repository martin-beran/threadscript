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

