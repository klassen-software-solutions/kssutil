//
//  rtti.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-01-22.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

/*!
 \file
 \brief Run Time Type Information.
 */

#if !defined(kssutil_rtti_h)
#define kssutil_rtti_h

/**
 * This file provides various rtti (runtime type information) utilities that expand
 * on the standard C++ abilities.
 */

#include <string>
#include <type_traits>
#include <typeinfo>

namespace kss { namespace util { namespace rtti {

    /*!
     Return the readable type name given a mangled one. Depending on the underlying
     architecture, this may throw a logic_error (Linux) or may core dump (Mac) if
     the type name given is not a valid mangled type name. Typically the type name
     will be the result of a typeid(something).name() call.
     @throws std::bad_alloc if the memory could not be allocated
     @throws std::runtime_error if the demangling fails
     */
    std::string demangle(const std::string& typeName);

    /*!
     Return the demangled name of the passed in type. This may be called in one
     of two ways, either
     `name(i)` where i is some variable of a potentially unknown type, or
     `name<TT>()` where TT is some type name.
     @throws any exception that demangle() may throw.
     */
    template <typename T>
    inline std::string name(const T& t = T()) {
        return demangle(typeid(t).name());
    }

    /**
     Returns true if t is a typeof C. This will be true if T and C are the same
     types or if T is a subclass of C. Note that the intended way to call this
     is to explicitly specify C. For example if B is a subclass of A then
     B b;
     isinstanceof<A>(b), and
     isinstanceof<B>(b) should both be true.

     @throws std::bad_typeid if t is a NULL pointer (pointer version only)
     */
    template <typename C, typename T>
    bool isInstanceOf(const T& t) noexcept {
        if (std::is_base_of<C, T>::value) {
            return true;
        }
        if (dynamic_cast<const C*>(&t) != nullptr) {
            return true;
        }
        return false;
    }

    template <typename C, typename T>
    bool isInstanceOfPtr(const T* t) {
        if (t == nullptr) {
            return false;
        }
        return isInstanceOf<C>(*t);
    }

    /*!
     If a pointer or reference to the type Base can be dynamically cast to the type T,
     return a pointer to the type T. Otherwise return nullptr.

     Note that the return type is always a pointer, even if the input is a reference.
     This is done to allow calling such as the following:

     @code
     MyBaseClass b;
     ...
     if (MySubClass* c = as<MySubClass>(b)) {
        ...do something with c...
     }
     @endcode
     */
    template <class T, class Base>
    T* as(Base* obj) noexcept {
        if (!obj) { return nullptr; }
        return dynamic_cast<T*>(obj);
    }

    template <class T, class Base>
    T* as(Base& obj) noexcept {
        return as<T, Base>(&obj);
    }

    template <class T, class Base>
    const T* as(const Base* obj) noexcept {
        if (!obj) { return nullptr; }
        return dynamic_cast<const T*>(obj);
    }

    template <class T, class Base>
    const T* as(const Base& obj) noexcept {
        return as<T, Base>(&obj);
    }

}}}

#endif
