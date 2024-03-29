/*! \file
 * \brief Tests of class threadscript::syntax::canon
 */

//! \cond
#include "threadscript/code_parser.hpp"

#define BOOST_TEST_MODULE syntax_canon
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace ts = threadscript;

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace test {

struct input {
    input(std::string text, size_t line = 0, size_t column = 0,
          std::string error = "Parse error"):
        text(std::move(text)), line(line), column(column),
        error(std::to_string(line) + ":" + std::to_string(column) + ": " +
              std::move(error))
    {}
    std::string text;
    size_t line;
    size_t column;
    std::string error;
};

std::ostream& operator<<(std::ostream& os, const input& v)
{
    os << '"' << v.text << "\"->" << v.line << ':' << v.column;
    return os;
}

struct parsed: input {
    parsed(std::string text, bool result = true, size_t line = 0,
           size_t column = 0, std::string error = "Parse error"):
        input(std::move(text), line, column, std::move(error)), result(result)
    {}
    bool result;
};

std::ostream& operator<<(std::ostream& os, const parsed& v)
{
    os << static_cast<const input&>(v) << ':' << (v.result ? "OK" : "FAIL");
    return os;
}

bool tracing_enabled()
{
    using namespace std::string_view_literals;
    int argc = boost::unit_test::framework::master_test_suite().argc;
    char** argv = boost::unit_test::framework::master_test_suite().argv;
    return argc > 1 && (argv[1] == "-t"sv || argv[1] == "--trace"sv);
}

void test_trace(std::optional<ts::parser::rule_result> result,
                const std::string& name, size_t depth, std::string_view error,
                size_t begin_line, size_t begin_column,
                size_t end_line, size_t end_column,
                std::optional<size_t> err_line,
                std::optional<size_t> err_column)
{
    if (tracing_enabled())
        std::cout <<
            ts::parser::context::trace_msg(result, name, depth, error,
                                           begin_line, begin_column, end_line,
                                           end_column, err_line, err_column) <<
            std::endl;
}

template <class Sample> void test_parse(Sample&& sample)
{
    test_parse(std::forward<Sample>(sample), [](){});
}

void test_parse(auto&& sample, auto&& check)
{
    ts::allocator_any alloc;
    if (tracing_enabled())
        std::cout << "input=\"" << sample.text << "\"" << std::endl;
    if (sample.result)
        BOOST_REQUIRE_NO_THROW(
            try {
                auto parsed = ts::parse_code(alloc, sample.text, "string",
                                             ts::syntax_factory::syntax_canon,
                                             test_trace);
                BOOST_CHECK_NE(parsed.get(), nullptr);
                check();
            } catch (std::exception& e) {
                BOOST_TEST_INFO("exception: " << e.what());
                throw;
            });
    else
        BOOST_CHECK_EXCEPTION(ts::parse_code(alloc, sample.text, "string",
                                             ts::syntax_factory::syntax_canon,
                                             test_trace),
            ts::parse_error,
            ([&sample](auto&& e) {
                BOOST_CHECK_EQUAL(e.what(), sample.error);
                BOOST_CHECK_EQUAL(e.pos().line, sample.line);
                BOOST_CHECK_EQUAL(e.pos().column, sample.column);
                return true;
            }));
}

} // namespace test
//! \endcond

/*! \file
 * \test \c parse -- Test parsing various valid and invalid source texts */
//! \cond
BOOST_DATA_TEST_CASE(parse, (std::vector<test::parsed>{
// Empty, spaces, comments
    {R"()", false, 1, 1, "Expected value or function"},
    {R"( )", false, 1, 2, "Expected value or function"},
    {R"(
# comment
    )", false, 3, 5, "Expected value or function"},
    {R"(
# comment begin
null
    )"},
    {R"(
# comment begin
null
# comment end
    )"},
    {R"(
# comment begin #1
# comment begin #2
null
 # comment end 3

# comment end 4
    )"},
    {R"(
null
# comment end
    )"},
// Values: null, bool
    {R"(null)"},
    {R"(false)"},
    {R"(true)"},
// Numbers (int, unsigned)
    {R"(123)"},
    {R"(+4)"},
    {R"(-56)"},
    {R"(+x4)", false, 1, 2, "Expected number"},
// Strings
    {R"("")"},
    {R"("a")"},
    {R"("abc")"},
    {R"("abcd)", false, 1, 6, "Expected '\"'"},
    {R"("\0\t\n\r\"\\")"},
    {R"("\x00\x01\xab\xcD\xcD\xEF\x2a\x31\xb3")"},
    {R"("\a")", false, 1, 2, "Expected '\"'"},
    {R"("\x0g")", false, 1, 2, "Expected '\"'"},
// Functions
    {R"(id)", false, 1, 3, "Expected '('"},
    {R"(id[])", false, 1, 3, "Expected '('"},
    {R"(id [])", false, 1, 4, "Expected '('"},
    {R"(id()", false, 1, 4, "Expected value or function"},
    {R"(id())"},
    {R"(a())"},
    {R"(A())"},
    {R"(_())"},
    {R"(abc())"},
    {R"(ABC())"},
    {R"(A23())"},
    {R"(123())", false, 1, 4, "Whitespace or comment expected"},
    {R"(id(1, 2))"},
    {R"(id(1, 2,))", false, 1, 9, "Expected value or function"},
    {R"(id(1, 2, null, false, true, "Hello"))"},
// Complex syntax
    {R"(id(1, f(2), g(null, false, true)))"},
    {R"(id(1, f(2), g(null, false, true), h1("Hello", h2("h3", 3))))"},
    {R"(id(1, f(2), g(null, false, true), h1("Hello", h2(h3, 3))))",
        false, 1, 52, "Expected '('"},
}))
{
    test_parse(sample);
}
//! \endcond
