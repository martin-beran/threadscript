#pragma once

/*! \file
 * \brief Helpers for templates and template parameters
 */

#include <algorithm>
#include <string_view>

namespace threadscript {

//! A helper class for using a string literal as a template parameter
/*! This class can be used as a non-type template parameter that accepts a
 * string literal as a template argument. Example:
 * \code
 * template <str_literal Name> class named_base {
 * public:
 *     [[nodiscard]] static consteval std::string_view type_name() {
 *         return Name;
 *     }
 * }
 *
 * class type_a: public named_base<"type_a"> {
 *     ...
 * }
 * \endcode
 * \tparam N the size of the stored string, usually deduced from a string
 * literal, as in the example above */
template <size_t N> class str_literal {
public:
    //! Stores a string
    /*! \param[in] str a string (usually a string literal to be stored in the
     * object */
    // NOLINTNEXTLINE(hicpp-explicit-conversions): we want implicit conversion
    constexpr str_literal(const char (&str)[N]) noexcept {
        std::copy_n(str, N, this->str);
    }
    //! Creates a std::string_view from the stored string \ref str
    /*! \return the stored string */
    // NOLINTNEXTLINE(hicpp-explicit-conversions): we want implicit conversion
    constexpr operator std::string_view() const noexcept {
        return str;
    }
    //! The stored string
    /*! \note This member is public, because the class must be structural in
     * order to be used as a non-type template parameter, and a structural type
     * must have all non-static data members public. */
    char str[N] = "";
};

} // namespace threadscript
