/*! \file
 * \brief The implementation part of debug.hpp
 */

#include "threadscript/debug.hpp"

#include <boost/process/environment.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <thread>

namespace threadscript {

thread_local bool DEBUG::active = false;

std::ostream* DEBUG::common_os = nullptr;

std::unique_ptr<std::ofstream> DEBUG::file_os = nullptr;

bool DEBUG::fmt_pid = false;

bool DEBUG::fmt_tid = false;

std::mutex DEBUG::mtx;

std::string DEBUG::prefix{};

#ifdef __cpp_lib_source_location
DEBUG::DEBUG(const std::source_location& loc):
#else
DEBUG::DEBUG(const boost::source_location& loc):
#endif
    lck(init()) 
{
    if (active) {
        assert(!lck);
        return;
    }
    assert(lck);
    active = true;
    os = common_os;
    if (os) {
        namespace sc = std::chrono;
        auto now =
            sc::time_point_cast<sc::microseconds>(sc::system_clock::now()).
            time_since_epoch().count();
        time_t sec = now / 1'000'000;
        int usec = int(now % 1'000'000);
        tm hms{};
        localtime_r(&sec, &hms);
        // hh:mm:ss.uuuuuu\0
        std::array<char, 16> t{};
        assert(snprintf(t.data(), t.size(), "%02d:%02d:%02d.%06d",
                        int(hms.tm_hour), int(hms.tm_min), int(hms.tm_sec),
                        usec) == t.size() - 1);
        std::string_view file = loc.file_name();
        if (auto i = file.rfind('/'); i != std::string_view::npos)
            file = file.substr(i + 1);
        *os << prefix;
        if (fmt_pid)
            *os << ' ' << boost::this_process::get_id();
        if (fmt_tid)
            * os << ' ' << std::this_thread::get_id();
        *os << ' ' << t.data() << ' ' << file << ':' << loc.line() << ' ';
    }
}

DEBUG::~DEBUG()
{
    if (lck) {
        std::move(*this) << std::endl<char, std::ostream::traits_type>;
        active = false;
    }
}

std::unique_lock<std::mutex> DEBUG::init()
{
    if (active)
        return {};
    static std::once_flag once;
    std::call_once(once, init_once);
    return std::unique_lock{mtx};
}

void DEBUG::init_once()
{
    using namespace std::string_view_literals;
    // NOLINTNEXTLINE(concurrency-mt-unsafe): MT access not expected
    char* p = std::getenv(env_var.data());
    std::string_view env = p ? p : "cerr"sv;
    if (env.empty())
        ; // not writing debugging messages
    else if (env == "cout"sv)
        common_os = &std::cout;
    else if (env == "cerr"sv)
        common_os = &std::cerr;
    else {
        file_os = std::make_unique<std::ofstream>(env.data(),
                                    std::ios_base::out | std::ios_base::app);
        common_os = file_os.get();
    }
    // NOLINTNEXTLINE(concurrency-mt-unsafe): MT access not expected
    p = std::getenv(env_var_format.data());
    prefix = "DBG";
    std::string_view fmt = p ? p : std::string_view{prefix};
    for (size_t i = 0; i < fmt.size(); ++i)
        switch (fmt[i]) {
        case 'p':
            fmt_pid = true;
            break;
        case 't':
            fmt_tid = true;
            break;
        case ' ':
        case ':':
            prefix = fmt.substr(i + 1);
            goto end_for;
        default:
            goto end_for;
        }
end_for:
    ;
}

} // namespace threadscript
