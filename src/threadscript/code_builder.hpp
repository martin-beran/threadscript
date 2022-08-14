#pragma once

/*! \file
 * \brief A non-template interface to code.hpp
 *
 * This file can be included by a script parser. It provides abstract
 * (interface) class script_builder containing functions usable for building a
 * script representation object threadscript::basic_script, independently of
 * its template parameter (an allocator). Implementation template
 * basic_code_builder_impl of the interface, dependent on the allocator, is
 * defined in code_builder_impl.hpp.
 */

#include "threadscript/configure.hpp"
#include "threadscript/exception.hpp"

namespace threadscript {

//! The interface to classes able to build parsed script representations
/*! An implementation of this abstract interface class defines functions that
 * create a basic_script object and populate it by basic_code_node objects,
 * using the allocator type of the instance of template basic_script. */
class script_builder {
public:
    //! An opaque handle to a script node
    class node_handle {
        //! Type-erased basic_script::node_ptr
        std::shared_ptr<const void> ptr;
        friend class script_builder; //!< Needs access to \ref ptr
    };
    //! An opaque handle to a value
    class value_handle {
    public:
        //! Default constructor
        value_handle() = default;
    private:
        //! Creates the handle object
        /*! \param[in] ptr the stored pointer */
        value_handle(const std::optional<std::shared_ptr<void>>& ptr):
            ptr(ptr) {}
        //! Type-erased basic_value::value_ptr
        std::optional<std::shared_ptr<void>> ptr;
        friend class script_builder; //!< Needs access to \ref ptr
    };
    //! Creates a new script builder
    script_builder() = default;
    //! No copy
    script_builder(const script_builder&) = delete;
    //! No move
    script_builder(script_builder&&) = delete;
    //! A virtual destructor, because this class is polymorphic
    virtual ~script_builder() = default;
    //! No copy
    script_builder& operator=(const script_builder&) = delete;
    //! No move
    script_builder& operator=(script_builder&&) = delete;
    //! Creates a new script object and stores it internally.
    /*! It may be called only once, the second call cases a failed assert.
     * \param[in] file a script file name, it will be stored in the created
     * script object */
    virtual void create_script(std::string_view file) = 0;
    //! Adds a new node to the script previously created by create_script()
    /*! Calling this function without a previous call to create_script() causes
     * a failed assert.
     * \param[in] parent the created node will be added as the root node if \a
     * parent is node_handle{}, otherwise it will be added as the last child of
     * \a parent
     * \param[in] location the location of the new node in the script
     * \param[in] name the name of the new node
     * \param[in] value the value of the new node
     * \return the new created node
     * \throw exception::parse_error if \a parent is \c nullptr and a _root
     * node already exists */
    virtual node_handle add_node(const node_handle& parent,
                                 const file_location& location,
                                 std::string_view name,
                                 const value_handle& value = {}) = 0;
    //! Creates a null value
    /*! \return basic_value_ptr with value \c nullptr */
    static value_handle create_value_null() {
        return value_handle{nullptr};
    }
    //! Creates a Boolean value
    /*! \param[in] val the value stored in the result
     * \return basic_value_ptr pointing to basic_value_bool */
    virtual value_handle create_value_bool(bool val) = 0;
    //! Creates a signed integer value
    /*! \param[in] val the value stored in the result
     * \return basic_value_ptr pointing to basic_value_int */
    virtual value_handle create_value_int(config::value_int_type val) = 0;
    //! Creates an unsigned integer value
    /*! \param[in] val the value stored in the result
     * \return basic_value_ptr pointing to basic_value_unsigned */
    virtual value_handle create_value_unsigned(config::value_unsigned_type val)
        = 0;
    //! Creates a string value
    /*! \param[in] val the value stored in the result
     * \return basic_value_ptr pointing to basic_value_string */
    virtual value_handle create_value_string(std::string_view val) = 0;
protected:
    //! Provides access to a script node for this and derived classes.
    /*! \param[in] hnd a handle to a node
     * \return type-erased basic_script::node_ptr */
    static std::shared_ptr<const void>& get(node_handle& hnd) noexcept {
        return hnd.ptr;
    }
    //! \copydoc get(node_handle&)
    static const std::shared_ptr<const void>&
    get(const node_handle& hnd) noexcept {
        return hnd.ptr;
    }
    //! Provides access to a value for this and derived classes.
    /*! \param[in] hnd a handle to a value
     * \return type-erased basic_value::value_ptr */
    static std::optional<std::shared_ptr<void>>& get(value_handle& hnd) noexcept
    {
        return hnd.ptr;
    }
    //! \copydoc get(value_handle&)
    static const std::optional<std::shared_ptr<void>>&
    get(const value_handle& hnd) noexcept {
        return hnd.ptr;
    }
};

} // namespace threadscript
