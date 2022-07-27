#pragma once

/*! \file
 * \brief Tools for generating temporary debugging messages
 */

#include <memory>
#include <mutex>
#include <ostream>
#include <source_location>
#include <string_view>

#undef __cpp_lib_source_location
#ifndef __cpp_lib_source_location
#include <boost/assert/source_location.hpp>
#endif

#ifdef DOXYGEN
//! Distinguish between a compiler and Doxygen.
/*! This macro is defined during Doxygen building documentation and undefined
 * during compilation. It can be used to selectively hide code from Doxygen or
 * from the compiler. */
#define DOXYGEN

//! It allows to use class threadscript::DEBUG.
/*! If this macro is not defined, any attempt to create a threadscript::DEBUG
 * instance is a compile time error. It can be used to make sure that all
 * temporary debugging messages have been removed in a non-debug build. It is
 * controlled by CMake. */
#define THREADSCRIPT_DEBUG
#endif

#ifndef THREADSCRIPT_DEBUG_ENV
//! The name of the environment variable that selects a debugging output.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THREADSCRIPT_DEBUG_ENV "THREADSCRIPT_DEBUG"
#endif

#ifndef THREADSCRIPT_DEBUG_FORMAT_ENV
//! The name of the environment variable specifying a debugging format.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THREADSCRIPT_DEBUG_FORMAT_ENV "THREADSCRIPT_DEBUG_FORMAT"
#endif

namespace threadscript {

//! An instance of this class is used to write a single debugging message.
/*! This class is intended to be used similarly to an output stream, using \c
 * operator<<() to compose a message. Values of any type, which has a stream
 * output \c operator<<(), can be written. Each message is terminated by a
 * newline and the output stream is flushed. Example:
 * \code
 * DEBUG() << "value=" << val;
 * \endcode
 * An instance of \ref DEBUG may be created only if macro THREADSCRIPT_DEBUG is
 * defined.
 *
 * Destination of debugging messages is controlled by the environment variable
 * which name is the value of macro THREADSCRIPT_DEBUG_ENV.
 * Possible values:
 * \arg The empty value means that debugging message will not be written.
 * \arg cout -- Write to \c std::cout
 * \arg cerr -- Write to \c std::cerr
 * \arg Any other value is taken as a file name, which will be opened for
 * appending.
 *
 * If the environment variable does not exist, the default value is \c cerr.
 * \threadsafe{safe\, each message is written atomically\, parts of messages
 * from different threads are not mixed in the output, unsafe}
 *
 * Formatting of messages is controlled by the environment variable which name
 * is the value of macro THREADSCRIPT_DEBUG_FORMAT_ENV. It contains zero or
 * more formatting flags:
 * \arg p -- Includes PID in each message.
 * \arg t -- Includes thread ID in each message.
 *
 * No flags are enabled by default. If the flags are followed by a space or
 * colon and some (possibly emtpy) text, this text is used as the message
 * prefix, instead of the default \c DBG.
 *
 * \note If several instances of \ref DEBUG exist in a thread at the same time,
 * only the one created first produces output. This rule prevents deadlocks or
 * garbled output if a \ref DEBUG is invoked recursively when generating a
 * debugging message, e.g., by a debugging message in an \c operator<<().
 *
 * \note Debugging messages generated after destroying file_os will not be
 * written. Such message can be written, e.g., by destructors of objects with
 * static storage duration destroyed after file_os.
 *
 * \note The name of this class is in upper case and does not follow the code
 * style rules in order to be easily found in source code. */
class DEBUG {
public:
    //! The name of the environment variable that selects a debugging output.
    static constexpr std::string_view env_var = THREADSCRIPT_DEBUG_ENV;
    //! The name of the environment variable that defines a message format.
    static constexpr std::string_view env_var_format =
        THREADSCRIPT_DEBUG_FORMAT_ENV;
#ifndef THREADSCRIPT_DEBUG
private: // Making constructors private makes this class unusable
#endif
    //! Default constructor.
    /*! \param[in] loc the location in the source code where this message is
     * generated */
#ifdef __cpp_lib_source_location
    // NOLINTNEXTLINE(modernize-use-equals-delete)
    explicit DEBUG(const std::source_location& loc =
                   std::source_location::current());
#else
    explicit DEBUG(const boost::source_location& loc = {});
#endif
public: // NOLINT(readability-redundant-access-specifiers)
    //! No copying
    DEBUG(const DEBUG&) = delete;
    //! No moving
    DEBUG(DEBUG&&) = delete;
    //! The destructor finishes writing the message.
    ~DEBUG();
    //! No copying
    DEBUG& operator=(const DEBUG&) = delete;
    //! No moving
    DEBUG& operator=(DEBUG&&) = delete;
private:
    //! Global initialization of debugging output
    /*! It opens the output stream and initializes a mutex for synchronization
     * of DEBUG() instances in different threads.
     * \return a lock to be stored in \ref lck */
    static std::unique_lock<std::mutex> init();
    //! An internal function used by init()
    static void init_once();
    std::unique_lock<std::mutex> lck; //!< The lock held by this object
    //! The output stream; \c nullptr if debugging output is disabled
    std::ostream* os = nullptr;
    //! Used to detect if a \ref DEBUG instance exists in the current thread.
    static thread_local bool active;
    static std::mutex mtx; //!< The mutex for synchronizing different instances
    //! The output stream; \c nullptr if debugging output is disabled.
    static std::ostream* common_os;
    //! The output file stream; \c nullptr if not using a file
    static std::unique_ptr<std::ofstream> file_os;
    static std::string prefix; //!< A message prefix
    static bool fmt_pid; //!< Add PID to messages
    static bool fmt_tid; //!< Add thread ID to messages
    //! Appends a value to the debugging message currently being built.
    /*! \tparam T the type of a written value
     * \param[in] dbg a debugging object
     * \param[in] v a value to be written
     * \return \a dbg */
    template <class T> friend DEBUG&& operator<<(DEBUG&& dbg, T&& v) {
        if (dbg.os)
            *dbg.os << std::forward<T>(v);
        return std::move(dbg);
    }
};

} // namespace threadscript
