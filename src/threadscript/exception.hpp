#pragma once

/*! \file
 * \brief Exception hierarchy used by the ThreadScript engine.
 *
 * Declarations in this file do not use custom allocators with limits, because
 * it must be possible to use them when allocation limits are exceeded, e.g.,
 * when reporting an allocation failure.
 */

#include <cassert>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace threadscript {

//! Location in a single source file
/*! \test in file test_exception.cpp */
struct file_location {
    //! An empty value, used for line and column numbers.
    static constexpr unsigned unknown = 0;
    //! Stores a location.
    /*! \param[in] line a line number
     * \param[in] column a column number */
    explicit file_location(unsigned line = unknown, unsigned column = unknown):
        line(line), column(column) {}
    //! Gets a file location as a string
    /*! \return the file location */
    [[nodiscard]] std::string to_string() const;
    //! Compares locations for equality
    /*! \param[in] o another location
     * \return whether the locations are equal */
    bool operator==(const file_location& o) const noexcept = default;
    //! The line containing the error
    /*! Line numbers start from 1; \ref unknown means that the line number
     * cannot be determined. */
    unsigned line = unknown;
    //! The position of the error on the \ref line
    /*! Column numbers start from 1; \ref unknown means that the exact
     * character position cannot be * determined. */
    unsigned column = unknown;
    //! Writes a source file to a stream.
    /*! It writes the result of to_string().
     * \param[in] os an output stream
     * \param[in] v a source location value to write
     * \return \c os */
    friend std::ostream& operator<<(std::ostream& os, const file_location& v) {
        os << v.to_string();
        return os;
    }
};

//! Location in the source code
/*! It adds a file name to file_location.
 * \test in file test_exception.cpp */
struct src_location: file_location {
    //! Stores a location.
    /*! \param[in] file a file name
     * \param[in] line a line number
     * \param[in] column a column number */
    explicit src_location(std::string_view file, unsigned line = unknown,
                          unsigned column = unknown):
        file_location(line, column), file(file) {}
    //! Stores a location without a file name.
    /*! \param[in] line a line number
     * \param[in] column a column number */
    explicit src_location(unsigned line = unknown, unsigned column = unknown):
        file_location(line, column) {}
    //! Gets a source location as a string
    /*! \return the source location */
    [[nodiscard]] std::string to_string() const;
    //! Name of the script source file
    /*! The empty string if the source location is not in a file, e.g.,
     * source code from the standard input or from a string. */
    std::string file;
    //! Writes a source location to a stream.
    /*! It writes the result of to_string().
     * \param[in] os an output stream
     * \param[in] v a source location value to write
     * \return \c os */
    friend std::ostream& operator<<(std::ostream& os, const src_location& v) {
        os << v.to_string();
        return os;
    }
};

//! Location on the stack
/*! It adds a function name to src_location.
 * \test in file test_exception.cpp */
struct frame_location: src_location {
    //! Stores a stack frame information.
    /*! \param[in] function a function name
        \param[in] file a file name
        \param[in] line a line number
        \param[in] column a column number */
    explicit frame_location(std::string_view function = {},
                            std::string_view file = {},
                            unsigned line = unknown, unsigned column = unknown):
        src_location(file, line, column), function(function) {}
    //! Gets a frame record as a string
    /*! \return the frame location */
    [[nodiscard]] std::string to_string() const;
    //! The function name related to ths stack frame
    /*! The empty string if the function name is not known. */
    std::string function;
    //! Writes a frame location to a stream.
    /*! It writes the result of to_string().
     * \param[in] os an output stream
     * \param[in] v a frame location value to write
     * \return \c os */
    friend std::ostream& operator<<(std::ostream& os, const frame_location& v) {
        os << v.to_string();
        return os;
    }
};

//! A stack trace.
/*! The most recently called function (the top of the stack) is at index 0.
 * \test in file test_exception.cpp */
class stack_trace: public std::vector<frame_location> {
    using std::vector<frame_location>::vector;
public:
    //! Gets the stored current location.
    /*! \return the frame at top of the stack, or an empty frame_location if
     * the stack trace is empty. */
    [[nodiscard]] frame_location location() const & noexcept {
        if (empty())
            return frame_location{"", ""};
        else
            return front();
    }
    //! Gets the stored current location.
    /*! \return the frame at top of the stack, or an empty frame_location if
     * the stack trace is empty. */
    [[nodiscard]] frame_location location() && noexcept {
        if (empty())
            return frame_location{"", ""};
        else
            return std::move(front());
    }
    //! Gets a stack trace as a (multiline) string
    /*! \param[in] full the full stack trace is written if \c true; only the
     * top of the stack is written if \c false
     * \return the stack trace */
    [[nodiscard]] std::string to_string(bool full = true) const;
    //! An I/O manipulator that requests a full stack trace in the next write.
    /*! \param[in] os an output stream
     * \return \a os */
    static std::ostream& full(std::ostream& os);
    //! Tests if full() has been applied to \a os and resets it to \c false.
    /*! \param[in] os an output stream
     * \return \c true if the next \c operator<<() should write the full stack
     * trace, \c false if it should write just the top of the stack */
    [[nodiscard]] static bool full_stream(std::ostream& os);
private:
    static const int full_idx; //!< Used by full() and full_stream()
    //! Writes a stack trace to a stream.
    /*! It writes the result of to_string(). If manipulator \ref full is
     * inserted into \a os, the next single call of \c operator<< will write
     * the full stack trace. Otherwise, only the top of the stack will be
     * written.
     * \param[in] os an output stream
     * \param[in] v a stack trace value to write
     * \return \c os */
    friend std::ostream& operator<<(std::ostream& os, const stack_trace& v) {
        os << v.to_string(stack_trace::full_stream(os));
        return os;
    }
};

//! A namespace containing all ThreadScript exceptions
/*! \test in file test_exception.cpp */
namespace exception {

//! The base class of all ThreadScript exceptions
class base: public std::runtime_error {
    using std::runtime_error::runtime_error;
public:
    //! Records an error description and a stack trace
    /*! \param[in] msg the error message (after the location)
     * \param[in] trace an optional stack trace */
    explicit base(std::string_view msg, stack_trace trace = {}):
        runtime_error(make_msg(msg, trace)),
        _trace(std::move(trace))
    {
        set_msg(msg.size());
    }
    //! Records a default message and a stack trace
    /*! \param[in] trace an optional stack trace */
    explicit base(stack_trace trace = {}):
        base(msg_default, std::move(trace)) {}
    //! Copy constructor
    /*! \param[in] o the source object */
    base(const base& o): runtime_error(o) {
        set_msg(o.msg().size());
    }
    //! Move constructor
    /*! \param[in] o the source object */
    base(base&& o) noexcept(noexcept(runtime_error(std::move(o)))):
        runtime_error(std::move(o))
    {
        // this is OK, even if what() result changes after move
        // NOLINTNEXTLINE(bugprone-use-after-move)
        set_msg(o.msg().size());
    }
    //! Default destructor
    ~base() override = default;
    //! Copy assignment
    /*! \param[in] o the source object
     * \return \c *this */
    base& operator=(const base& o) {
        if (&o != this) {
            runtime_error::operator=(o);
            set_msg(o.msg().size());
        }
        return *this;
    }
    //! Move assignment
    /*! \param[in] o the source object
     * \return \c *this */
    base& operator=(base&& o)
        noexcept(noexcept(runtime_error::operator=(std::move(o))))
    {
        if (&o != this) {
            runtime_error::operator=(std::move(o));
            // NOLINTNEXTLINE(bugprone-use-after-move)
            set_msg(o.msg().size());
        }
        return *this;
    }
    //! Gets the exception type name.
    /*! This member function must be overriden in each derived class.
     * \return the class name without namespace */
    [[nodiscard]] virtual std::string_view type() const noexcept {
        return "base";
    }
    //! Creates the message to be stored in the exception.
    /*! \param[in] msg the error message (after the location)
     * \param[in] trace a stack trace
     * \return the message */
    static std::string make_msg(std::string_view msg, const stack_trace& trace);
    //! Replaces the stored stack trace
    /*! \param[in] trace a stack trace */
    void set_trace(stack_trace trace);
    //! Gets the stored part of the message after the location.
    /*! \return the message */
    [[nodiscard]] std::string_view msg() const noexcept {
        return _msg;
    }
    //! Gets the stored error location.
    /*! \return the frame at top of the stack, or an empty frame_location if
     * the stack trace is empty. */
    [[nodiscard]] frame_location location() const & noexcept {
        return _trace.location();
    }
    //! Gets the stored error location.
    /*! \return the frame at top of the stack, or an empty frame_location if
     * the stack trace is empty. */
    [[nodiscard]] frame_location location() && noexcept {
        return std::move(_trace).location();
    }
    //! Gets the stored stack trace.
    /*! \return the stack trace */
    [[nodiscard]] const stack_trace& trace() const & noexcept {
        return _trace;
    }
    //! Gets the stored stack trace.
    /*! \return the stack trace */
    [[nodiscard]] stack_trace trace() && noexcept {
        return std::move(_trace);
    }
    //! The same as in the base class
    /*! If the stack trace in nonempty, the error message will contain the top
     * of the stack location. If the stack trace is empty, the error message
     * will be equal to the return value of msg().
     * It is marked final, because msg() and set_msg() depend on its behavior.
     * \return the error message */
    [[nodiscard]] const char* what() const noexcept override final {
        return runtime_error::what();
    }
    //! Gets the result of what() optionally followed by a stack trace.
    /*! \param[in] full if the full stack trace is also returned
     * \return the error message and optionally the stack trace */
    [[nodiscard]] std::string to_string(bool full = true) const;
private:
    //! The default text of the error message
    static constexpr const char* msg_default = "ThreadScript exception";
    //! Sets _msg to the last \a sz characters of the string returned by what().
    /*! \param[in] sz the expected size of _msg */
    void set_msg(size_t sz);
    std::string_view _msg; //!< The stored message
    stack_trace _trace; //!< The recorded stack trace
    //! Writes the exception to a stream.
    /*! It writes the result of what(). It honors manipulator
     * stack_trace::full().
     * \param[in] os an output stream
     * \param[in] v a frame location value to write
     * \return \c os */
    friend std::ostream& operator<<(std::ostream& os, const base& v) {
        os << v.to_string(stack_trace::full_stream(os));
        return os;
    }
};

//! A wrapped exception
/*! It can be use to wrap another exception, e.g., an exception class defined
 * in the standard library, in a ThreadScript exception (with a stack trace).
 * It uses \c std::current_exception() to get the exception to be wrapped,
 * therefore it \e must be called when a current exception exists (typically,
 * in a \c catch). The wrapped exception can be unwrapped and rethrown. */
class wrapped: public base {
public:
    //! Stores the current exception and a stack trace.
    /*! \param[in] trace a stack trace */
    explicit wrapped(stack_trace trace = {}):
        wrapped(msg_default, std::move(trace)) {}
    //! Stores the current exception, a message, and a stack trace.
    /*! \param[in] exc an exception to extract the message from
     * \param[in] trace a stack trace */
    explicit wrapped(const std::exception& exc, stack_trace trace = {}):
        wrapped(exc.what(), std::move(trace)) {}
    //! Stores the current exception, a message, and a stack trace.
    /*! \param[in] msg a message
     * \param[in] trace a stack trace */
    explicit wrapped(std::string_view msg, stack_trace trace);
    [[nodiscard]] std::string_view type() const noexcept override {
        return "wrapped";
    }
    //! Throws the wrapped exception.
    [[noreturn]] void rethrow() { std::rethrow_exception(_wrapped); }
private:
    //! The default text of the error message
    static constexpr const char* msg_default = "ThreadScript wrapped exception";
    std::exception_ptr _wrapped; //!< The wrapped exception
};

//! An exception thrown when an unimplemented feature is invoked
class not_implemented: public base {
public:
    //! Stores an error message.
    /*! \param[in] feature the unimplemented feature name
     * \param[in] trace a stack trace */
    explicit not_implemented(std::string_view feature,
                             stack_trace trace = {}):
        base(std::string(feature).append(" not implemented"), std::move(trace))
    {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "not_implemented";
    }
};

//! An error detected when parsing a script source
class parse_error: public base {
public:
    //! Stores an error message.
    /*! \param[in] msg a description of a parsing error
     * \param[in] trace a stack trace */
    explicit parse_error(std::string_view msg, stack_trace trace = {}):
        base(std::string("Parse error: ").append(msg), std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "parse_error";
    }
};

//! An error detected when running a script
class runtime_error: public base {
protected:
    //! Stores an error message.
    /*! \param[in] msg a description of a runtime error
     * \param[in] trace a stack trace */
    explicit runtime_error(std::string_view msg, stack_trace trace = {}):
        base(std::string("Runtime error: ").append(msg), std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "runtime_error";
    }
};

//! A memory allocation error
/*! It must not be instantiated, use a derived class instead. */
class alloc: public runtime_error {
protected:
    //! Stores an error message.
    /*! \param[in] msg a description of an allocation error
     * \param[in] trace a stack trace */
    explicit alloc(std::string_view msg, stack_trace trace = {}):
        runtime_error(msg, std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "alloc";
    }
};

//! A failed underlying memory allocation
/*! It is thrown if an underlying memory allocation mechanism fails, usually
 * throwing \c std::bad_alloc. */
class alloc_bad: public alloc {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit alloc_bad(stack_trace trace = {}):
        alloc("Allocation failed", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "alloc_bad";
    }
};

//! A memory allocation denied by a limit
class alloc_limit: public alloc {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit alloc_limit(stack_trace trace = {}):
        alloc("Allocation denied by limit", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "alloc_limit";
    }
};

//! A symbol name does not exist.
class unknown_symbol: public runtime_error {
public:
    //! Stores an error message.
    /*! \param[in] name the unknown symbol name
     * \param[in] trace a stack trace */
    explicit unknown_symbol(std::string_view name, stack_trace trace = {}):
        runtime_error(std::string("Symbol not found: ").append(name),
                      std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "unknown_symbol";
    }
};

//! A bad value passed as an argument to an operation.
/*! It must not be instantiated, use a derived class instead. */
class value: public runtime_error {
protected:
    //! Stores an error message.
    /*! \param[in] msg a description of an allocation error
     * \param[in] trace a stack trace */
    explicit value(std::string_view msg, stack_trace trace = {}):
        runtime_error(msg, std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "value";
    }
};

//! A bad content of a value
/*! It is used if a value cannot be used for an intended operation and there is
 * no more specific exception class. */
class value_bad: public value {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit value_bad(stack_trace trace = {}):
        value("Bad value", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "value_bad";
    }
};

//! An unexpected \c null value
class value_null: public value {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit value_null(stack_trace trace = {}):
        value("Null value", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "value_null";
    }
};

//! Trying to modify a read-only value
class value_read_only: public value {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit value_read_only(stack_trace trace = {}):
        value("Read-only value", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "value_read_only";
    }
};

//! Trying to mark as mt-safe a value that does not satisfy conditions
class value_mt_unsafe: public value {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit value_mt_unsafe(stack_trace trace = {}):
        value("Thread-unsafe value", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "value_mt_unsafe";
    }
};

//! A bad type of a value
class value_type: public value {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit value_type(stack_trace trace = {}):
        value("Bad value type", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "value_type";
    }
};

//! Accessing an element out of range
/*! It is similar to \c std::out_of_range. It is used for errors like an index
 * out of range of a vector or a key not existing in a hash. */
class value_out_of_range: public value {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit value_out_of_range(stack_trace trace = {}):
        value("Value out of range", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "value_out_of_range";
    }
};

//! An error detected during an operation
/*! It must not be instantiated, use a derived class instead. */
class operation: public runtime_error {
protected:
    //! Stores an error message.
    /*! \param[in] msg a description of an allocation error
     * \param[in] trace a stack trace */
    explicit operation(std::string_view msg, stack_trace trace = {}):
        runtime_error(msg, std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "operation";
    }
};

//! A bad operation.
/*! It is used if an operation cannot be performed and there is no more specific
 * exception class. */
class op_bad: public operation {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit op_bad(stack_trace trace = {}):
        operation("Bad operation", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "op_bad";
    }
};

//! Too deep recursion of function calls.
/*! It is used if the nesting level of a function call exceeds a limit. */
class op_recursion: public operation {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit op_recursion(stack_trace trace = {}):
        operation("Recursion too deep", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "op_recursion";
    }
};

//! Invalid number of arguments of an operation
/*! It is used if a command or a function is called with a wrong number of
 * arguments. */
class op_narg: public operation {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit op_narg(stack_trace trace = {}):
        operation("Bad number of arguments", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "op_narg";
    }
};

//! The result of an operation is out of range of the destination value_type.
/*! It is used, e.g., to signal overflow of an arithmetic operation with signed
 * values. */
class op_overflow: public operation {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit op_overflow(stack_trace trace = {}):
        operation("Overflow", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "op_overflow";
    }
};

//! Division by zero.
class op_div_zero: public operation {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit op_div_zero(stack_trace trace = {}):
        operation("Division by zero", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "op_div_zero";
    }
};

//! Thrown by a non-blocking operation, if its blocking counterpart would block.
class op_would_block: public operation {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit op_would_block(stack_trace trace = {}):
        operation("Operation would block", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "op_would_block";
    }
};

//! A failed call to an OS or library function.
/*! It is used if there is no more specific exception class. */
class op_library: public operation {
public:
    //! Stores an error message.
    /*! \param[in] trace a stack trace */
    explicit op_library(stack_trace trace = {}):
        operation("Library failure", std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "op_library";
    }
};

//! An exception thrown by command \c throw from a script
/*! It is used to represent exceptions explicitly thrown by a script command \c
 * throw (implemented by class predef::f_throw). */
class script_throw: public base {
public:
    //! The fixed prefix of a message
    static constexpr std::string_view prefix{"Script exception: "};
    //! Stores an error message.
    /*! \param[in] msg the message passed to predef::f_throw
     * \param[in] trace a stack trace */
    explicit script_throw(std::string_view msg, stack_trace trace = {}):
        base(std::string(prefix).append(msg), std::move(trace)) {}
    [[nodiscard]] std::string_view type() const noexcept override {
        return "script_throw";
    }
    //! Gets the stored part of the message after the \ref prefix.
    /*! \return the message as set by predef::f_throw */
    [[nodiscard]] std::string_view script_msg() const noexcept {
        assert(msg().size() >= prefix.size());
        return msg().substr(prefix.size());
    }
};

} // namespace exception

} // namespace threadscript
