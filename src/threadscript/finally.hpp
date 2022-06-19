#pragma once

/*! \file
 * \brief A RAII function wrapper \ref threadscript::finally.
 */

#include "threadscript/concepts.hpp"

#include <utility>

namespace threadscript {

//! A RAII function wrapper for a function that should be called at a scope end
/*! It calls a function when the scope contaning an automatic variable of class
 * \ref finally is ended, either normally or by an exception.
 * \tparam F the type of the called function */
template <impl::finally_fun F> class finally {
public:
    //! Registers a function to be called at a scope end.
    /*! \param[in] f a function */
    explicit finally(F&& f) noexcept: f(std::move(f)) {}
    //! No copying
    finally(const finally&) = delete;
    //! No moving
    finally(finally&&) = delete;
    //! No copying
    finally& operator=(const finally&) = delete;
    //! No moving
    finally& operator=(finally&&) = delete;
    //! Calls the registered function
    ~finally() {
        f();
    }
private:
    F f; //!< The registered function
};

} // namespace threadscript
