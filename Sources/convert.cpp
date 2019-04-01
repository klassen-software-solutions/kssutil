//
//  convert.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2018-06-15.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//
//  Permission is hereby granted to use/modify/publish this code without restriction or
//  requirement other than you are not allowed to hinder others from doing the same.
//  No warranty or other guarantee is given regarding the use of this code.
//

#include <cassert>
#include <cstdlib>
#include <functional>
#include <limits>

#include "_contract.hpp"
#include "convert.hpp"
#include "stringutil.hpp"
#include "timeutil.hpp"

using namespace std;
using namespace std::chrono;
using namespace kss::util::strings;
using kss::util::time::checkedDurationCast;
namespace contract = kss::util::_private::contract;


// MARK: Simple type overrides

namespace {
    template <class T>
    void throwException(int error, const string& s) {
        const auto typeName = kss::util::rtti::name<T>();
        throw system_error(error, system_category(),
                           "Could not convert '" + s + "' to " + typeName);
    }

    template <class T>
    T doConvert(const string& s, function<T(const char* sptr, char** endptr)> fn) {
        contract::parameters({
            KSS_EXPR(!s.empty())
        });

        errno = 0;
        char* endptr = nullptr;
        const T t = fn(s.c_str(), &endptr);

        if (!errno && (s.c_str() == endptr)) { throwException<T>(EINVAL, s); }
        if (errno) { throwException<T>(errno, s); }

        return t;
    }

    template <class T>
    T doConvertInt(const string& s, function<T(const char*, char**, int)> fn) {
        return doConvert<T>(s, [&](const char* sptr, char** endptr) {
            return fn(sptr, endptr, 0);
        });
    }

    template <class T>
    T doConvertUnsigned(const string& s, function<T(const char*, char**, int)> fn) {
        const auto pos = s.find_first_not_of("\t\n\v\f\r ");
        if (pos != string::npos && s[pos] == '-') {
            throwException<T>(EINVAL, s);
        }
        return doConvertInt<T>(s, fn);
    }
}

template<>
float kss::util::strings::convert(const string& s, const float&) {
    return doConvert<float>(s, strtof);
}

template<>
double kss::util::strings::convert(const string& s, const double&) {
    return doConvert<double>(s, strtod);
}

template<>
long double kss::util::strings::convert(const string& s, const long double&) {
    return doConvert<long double>(s, strtold);
}

template<>
int kss::util::strings::convert(const string& s, const int&) {
    const auto val = convert<long>(s);
    if (val < numeric_limits<int>::min() || val > numeric_limits<int>::max()) {
        throwException<int>(ERANGE, s);
    }
    return (int)val;
}

template<>
long kss::util::strings::convert(const string& s, const long&) {
    return doConvertInt<long>(s, strtol);
}

template<>
long long kss::util::strings::convert(const string& s, const long long&) {
    return doConvertInt<long>(s, strtoll);
}

template<>
unsigned kss::util::strings::convert(const string& s, const unsigned&) {
    const auto val = convert<unsigned long>(s);
    if (val > numeric_limits<unsigned>::max()) {
        throwException<unsigned>(ERANGE, s);
    }
    return (unsigned)val;
}

template<>
unsigned long kss::util::strings::convert(const string& s, const unsigned long&) {
    return doConvertUnsigned<unsigned long>(s, strtoul);
}

template<>
unsigned long long kss::util::strings::convert(const string& s, const unsigned long long&) {
    return doConvertUnsigned<unsigned long long>(s, strtoull);
}

// MARK: Duration overrides

namespace {
    template <class SourceDuration, class TargetDuration>
    TargetDuration doConvertKnownDuration(const string& s) {
        const auto count = convert<typename SourceDuration::rep>(s);
        return checkedDurationCast<TargetDuration>(SourceDuration(count));
    }

    template <class Duration>
    Duration doConvertDuration(const string& s, const Duration& = Duration()) {
        contract::parameters({
            KSS_EXPR(!s.empty())
        });

        if (endsWith(s, "ns")) {
            return doConvertKnownDuration<nanoseconds, Duration>(s);
        }
        else if (endsWith(s, "us")) {
            return doConvertKnownDuration<microseconds, Duration>(s);
        }
        else if (endsWith(s, "ms")) {
            return doConvertKnownDuration<milliseconds, Duration>(s);
        }
        else if (endsWith(s, "s")) {
            return doConvertKnownDuration<seconds, Duration>(s);
        }
        else if (endsWith(s, "min")) {
            return doConvertKnownDuration<minutes, Duration>(s);
        }
        else if (endsWith(s, "h")) {
            return doConvertKnownDuration<hours, Duration>(s);
        }
        else {
            throwException<Duration>(EINVAL, s);
            throw runtime_error("This line will never be run!");
        }
    }
}

template<>
hours kss::util::strings::convert(const string& s, const hours& typeArg) {
    return doConvertDuration(s, typeArg);
}

template<>
minutes kss::util::strings::convert(const string& s, const minutes& typeArg) {
    return doConvertDuration(s, typeArg);
}

template<>
seconds kss::util::strings::convert(const string& s, const seconds& typeArg) {
    return doConvertDuration(s, typeArg);
}

template<>
milliseconds kss::util::strings::convert(const string& s, const milliseconds& typeArg) {
    return doConvertDuration(s, typeArg);
}

template<>
microseconds kss::util::strings::convert(const string& s, const microseconds& typeArg) {
    return doConvertDuration(s, typeArg);
}

template<>
nanoseconds kss::util::strings::convert(const string& s, const nanoseconds& typeArg) {
    return doConvertDuration(s, typeArg);
}

// MARK: time_point overrides

namespace {
    template <class Clock, class Duration>
    inline time_point<Clock, Duration>
    doConvertTimePoint(const string& s, const time_point<Clock, Duration>&)
    {
        contract::parameters({
            KSS_EXPR(!s.empty())
        });
        return kss::util::time::fromIso8601String<time_point<Clock, Duration>>(s);
    }
}

template<>
time_point<system_clock, hours>
kss::util::strings::convert(const std::string& s, const time_point<system_clock, hours>& tp) {
    return doConvertTimePoint(s, tp);
}

template<>
time_point<system_clock, minutes>
kss::util::strings::convert(const std::string& s, const time_point<system_clock, minutes>& tp)
{
    return doConvertTimePoint(s, tp);
}

template<>
time_point<system_clock, seconds>
kss::util::strings::convert(const std::string& s, const time_point<system_clock, seconds>& tp)
{
    return doConvertTimePoint(s, tp);
}

template<>
time_point<system_clock, milliseconds>
kss::util::strings::convert(const std::string& s,
                            const time_point<system_clock, milliseconds>& tp)
{
    return doConvertTimePoint(s, tp);
}

template<>
time_point<system_clock, microseconds>
kss::util::strings::convert(const std::string& s,
                            const time_point<system_clock, microseconds>& tp)
{
    return doConvertTimePoint(s, tp);
}

template<>
time_point<system_clock, nanoseconds>
kss::util::strings::convert(const std::string& s,
                            const time_point<system_clock, nanoseconds>& tp)
{
    return doConvertTimePoint(s, tp);
}


template<>
time_point<steady_clock, hours>
kss::util::strings::convert(const std::string& s, const time_point<steady_clock, hours>& tp) {
    return doConvertTimePoint(s, tp);
}

template<>
time_point<steady_clock, minutes>
kss::util::strings::convert(const std::string& s,
                            const time_point<steady_clock, minutes>& tp)
{
    return doConvertTimePoint(s, tp);
}

template<>
time_point<steady_clock, seconds>
kss::util::strings::convert(const std::string& s,
                            const time_point<steady_clock, seconds>& tp)
{
    return doConvertTimePoint(s, tp);
}

template<>
time_point<steady_clock, milliseconds>
kss::util::strings::convert(const std::string& s,
                            const time_point<steady_clock, milliseconds>& tp)
{
    return doConvertTimePoint(s, tp);
}

template<>
time_point<steady_clock, microseconds>
kss::util::strings::convert(const std::string& s,
                            const time_point<steady_clock, microseconds>& tp)
{
    return doConvertTimePoint(s, tp);
}

template<>
time_point<steady_clock, nanoseconds>
kss::util::strings::convert(const std::string& s,
                            const time_point<steady_clock, nanoseconds>& tp)
{
    return doConvertTimePoint(s, tp);
}
