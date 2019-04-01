//
// FILENAME:    error.hpp
// AUTHOR:      Steven Klassen
// CREATED ON:  2011-08-01
//
// DESCRIPTION: Error handling in the KSS library.
//
// This file is Copyright (c) 2011 by Klassen Software Solutions. All rights reserved.
// Licensing follows the MIT License.
//

/*!
 \file
 \brief Miscellaneous algorithms related to error handling.
 */

#if !defined(kssutil_error_hpp)
#define kssutil_error_hpp

#include <exception>
#include <functional>
#include <string>
#include <utility>

namespace kss { namespace util {

    /*!
     tryAll is used to convert exceptions into more of a "C"-style error handling.
     This is useful if you don't care what the error was and just want to know if it
     worked.

     The non-template version will return true if everything works (i.e. if there is
     no exception thrown), and false if an exception is thrown.

     Also note that this will only catch exceptions subclassed from std::exception.
     Any other exceptions will call std::terminate().

     The template version will return a pair<T, bool>. If no exception is thrown, then
     T will be the value returned by the functional and bool will be true. If an
     exception is thrown, then T will be the value of an empty T() and the bool will
     be false.

     Note the following limitation: In order to ensure a level of efficiency, the
     template version will only compile if T supports std::move.

     @param fn The code block (or function) that is to be run.
     */
    bool tryAll(const std::function<void()>& fn) noexcept;

    template <class T>
    std::pair<T, bool> tryAll(const std::function<T()>& fn) noexcept {
        try {
            return std::make_pair(std::move(fn()), true);
        }
        catch (const std::exception&) {
            return std::make_pair(T(), false);
        }
    }

    /*!
     Returns a description of an exception that includes the name of the exception and
     the value returned by its what() method, e.g. "std::runtime_error: this is a test".

     In the "special case" of a system_error using the system_category(), the description
     will also include the value of the error number,
     e.g. "std::system_error: (12) Cannot allocate memory".

     @throws std::bad_alloc if the memory could not be allocated
     */
    std::string errorDescription(const std::exception& e);

}}

#endif
