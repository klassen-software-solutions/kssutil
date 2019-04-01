//
//  convert.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2018-06-15.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssutil_convert_hpp
#define kssutil_convert_hpp

#include <cerrno>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <system_error>
#include <typeinfo>

#include "rtti.hpp"

/*!
 \file
 \brief Convert strings to other types.
 */

namespace kss { namespace util { namespace strings {

    /*!
     Attempt to convert a string to the type T.
     @throws std::system_error if the conversion failed.
     @throws std::invalid_argument if s is empty
     */
    template <class T>
    T convert(const std::string& s, const T& = T()) {
        if (s.empty()) {
            throw std::invalid_argument("s cannot be empty");
        }

        std::istringstream strm(s);
        T t;
        strm >> t;
        if (strm.bad() || strm.fail()) {
            throw std::system_error(EIO, std::system_category(),
                                    "Could not convert '" + s + "'"
                                    + " to the type " + kss::util::rtti::name<T>());
        }
        return t;
    }

    template<>
    inline std::string convert(const std::string& s, const std::string&) {
        return s;
    }

    // These specializations use implementations from <cstdlib> as they should be
    // more efficient than using the stream conversions.
    template<> float convert(const std::string& s, const float&);
    template<> double convert(const std::string& s, const double&);
    template<> long double convert(const std::string& s, const long double&);
    template<> int convert(const std::string& s, const int&);
    template<> long convert(const std::string& s, const long&);
    template<> long long convert(const std::string& s, const long long&);
    template<> unsigned convert(const std::string& s, const unsigned&);
    template<> unsigned long convert(const std::string& s, const unsigned long&);
    template<> unsigned long long convert(const std::string& s,
                                          const unsigned long long&);

    // These specializations are used to convert text representations of durations
    // (e.g. "10s") into their corresponding durations (e.g. std::chrono::seconds).
    // Note that while these will convert the duration types, for example,
    //   seconds oneMinute = convert("1m");
    // is perfectly valid and will be equivalent to 60s, they will throw an
    // std::overflow_error if the resulting value cannot be represented by the desired
    // type.
    template<> std::chrono::hours convert(const std::string& s, const std::chrono::hours&);
    template<> std::chrono::minutes convert(const std::string& s, const std::chrono::minutes&);
    template<> std::chrono::seconds convert(const std::string& s, const std::chrono::seconds&);
    template<> std::chrono::milliseconds convert(const std::string& s, const std::chrono::milliseconds&);
    template<> std::chrono::microseconds convert(const std::string& s, const std::chrono::microseconds&);
    template<> std::chrono::nanoseconds convert(const std::string& s, const std::chrono::nanoseconds&);

    // These specializations are used to convert text representations of time, in
    // ISO8601 format, into time_point values. This covers the "standard" time_point
    // definitions. If you have your own you will need to provide your own overrides.
    template<>
    std::chrono::time_point<std::chrono::system_clock, std::chrono::hours>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::system_clock, std::chrono::hours>&);

    template<>
    std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>&);

    template<>
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>&);

    template<>
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>&);

    template<>
    std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>&);

    template<>
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>&);


    template<>
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::hours>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::steady_clock, std::chrono::hours>&);

    template<>
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::minutes>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::steady_clock, std::chrono::minutes>&);

    template<>
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>&);

    template<>
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>&);

    template<>
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>&);

    template<>
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>
    convert(const std::string& s, const std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>&);
}}}

#endif
