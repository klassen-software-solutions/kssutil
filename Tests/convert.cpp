//
//  convert.cpp
//  kssutiltest
//
//  Created by Steven W. Klassen on 2018-06-18.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <functional>
#include <iostream>
#include <istream>
#include <limits>
#include <sstream>
#include <string>

#include <kss/test/all.h>
#include <kss/util/convert.hpp>

using namespace std;
using namespace kss::util::strings;
using namespace kss::test;


namespace {
    class MyCustomClass {
    public:
        int value() const noexcept { return _val; }
        void setValue(int val) { _val = val; }

    private:
        int _val;
    };

    istream& operator>>(istream& strm, MyCustomClass& mcc) {
        int v = 0;
        strm >> v;
        mcc.setValue(v);
        return strm;
    }

    bool checkForSystemError(function<void(void)> fn) {
        try {
            fn();
            return false;       // no exception thrown
        }
        catch (const system_error&) {
            return true;        // correct exception thrown
        }
        catch (...) {
            return false;       // incorrect exception thrown
        }
    }

    bool checkForInvalidArgument(function<void(void)> fn) {
        try {
            fn();
            return false;       // no exception thrown
        }
        catch (const invalid_argument&) {
            return true;        // correct exception thrown
        }
        catch (...) {
            return false;       // incorrect exception thrown
        }
    }


    bool checkForOverflowError(function<void(void)> fn) {
        try {
            fn();
            return false;       // no exception thrown
        }
        catch (const system_error& e) {
            if (e.code().value() != ERANGE) {
                return false;   // incorrect error code
            }
            return true;        // correct exception thrown
        }
        catch (...) {
            return false;       // incorrect exception thrown
        }
    }

    // Check that a convert does the following:
    //  convert<T>(strValue) returns the correct value,
    //  convert<T>("hello") throws the correct exception,
    //  convert<T>("") throws the correct exception, and
    //  (optionally) convert<T>("-1") throws the correct exception
    template <class T>
    bool checkConvert(const string& strValue, T value, bool addUnsignedCheck = false) {
        const auto t = convert<T>(strValue);
        if (t != value) {
            return false;
        }

        if (!checkForSystemError([]{ convert<T>("hello"); })) return false;
        if (!checkForInvalidArgument([]{ convert<T>(""); })) return false;
        if (addUnsignedCheck) {
            if (!checkForSystemError([]{ convert<T>("-1"); })) return false;
        }

        return true;
    }

    template <class Clock, class Duration>
    bool checkConvertTimePoint(Duration offset) {
        return checkConvert("1972-04-13T08:00:00Z", chrono::time_point<Clock, Duration>(offset));
    }
}

static TestSuite ts("strings::convert", {
    make_pair("default convert", [] {
        KSS_ASSERT(isEqualTo<int>(12, [] {
            return convert<MyCustomClass>("12").value();
        }));

        KSS_ASSERT(throwsException<system_error>([] {
            convert<MyCustomClass>("hello");
        }));

        KSS_ASSERT(throwsException<invalid_argument>([] {
            convert<MyCustomClass>("");
        }));
    }),
    make_pair("specialization", [] {
        KSS_ASSERT(convert<string>("hello world!") == "hello world!");
        KSS_ASSERT(checkConvert("1.5", 1.5F));
        KSS_ASSERT(checkConvert("1.5", 1.5));
        KSS_ASSERT(checkConvert("1.5", 1.5L));
        KSS_ASSERT(checkConvert("15", 15));
        KSS_ASSERT(checkConvert("15", 15L));
        KSS_ASSERT(checkConvert("15", 15LL));
        KSS_ASSERT(checkConvert("15", 15U, true));
        KSS_ASSERT(checkConvert("15", 15UL, true));
        KSS_ASSERT(checkConvert("15", 15ULL, true));
    }),
    make_pair("overflows", [] {
        // Note it is technically possible for int (and unsigned) to be the same size
        // as long long in which case overflows would not be possible and this test
        // would be incorrect.

        if (sizeof(long long) > sizeof(int)) {
            const auto reallyBigValue = to_string(numeric_limits<long long>::max());
            const auto reallySmallValue = to_string(numeric_limits<long long>::min());
            KSS_ASSERT(checkForOverflowError([&]{ convert<int>(reallyBigValue); }));
            KSS_ASSERT(checkForOverflowError([&]{ convert<int>(reallySmallValue); }));
        }

        if (sizeof(unsigned long long) > sizeof(unsigned)) {
            const auto reallyBigValue = to_string(numeric_limits<unsigned long long>::max());
            KSS_ASSERT(checkForOverflowError([&]{ convert<unsigned>(reallyBigValue); }));
        }
    }),
    make_pair("duration", [] {
        KSS_ASSERT(checkConvert<chrono::hours>("10h", 10h));
        KSS_ASSERT(checkConvert<chrono::minutes>("10min", 10min));
        KSS_ASSERT(checkConvert<chrono::seconds>("10s", 10s));
        KSS_ASSERT(checkConvert<chrono::milliseconds>("10ms", 10ms));
        KSS_ASSERT(checkConvert<chrono::microseconds>("10us", 10us));
        KSS_ASSERT(checkConvert<chrono::nanoseconds>("10ns", 10ns));

        KSS_ASSERT(throwsException<system_error>([] {
            convert<chrono::hours>("10");
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            convert<chrono::minutes>("10");
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            convert<chrono::seconds>("10");
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            convert<chrono::milliseconds>("10");
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            convert<chrono::microseconds>("10");
        }));
        KSS_ASSERT(throwsException<system_error>([] {
            convert<chrono::nanoseconds>("10");
        }));

        KSS_ASSERT(checkConvert<chrono::seconds>("1h", 3600s));
        KSS_ASSERT(checkConvert<chrono::seconds>("1min", 60s));
        KSS_ASSERT(checkConvert<chrono::nanoseconds>("1s", 1000000000ns));
        KSS_ASSERT(checkConvert<chrono::nanoseconds>("1ms", 1000000ns));
        KSS_ASSERT(checkConvert<chrono::nanoseconds>("1us", 1000ns));

        KSS_ASSERT(throwsException<overflow_error>([] {
            ostringstream tmp;
            tmp << chrono::hours::max().count() << "h";
            convert<chrono::nanoseconds>(tmp.str());
        }));
    }),
    make_pair("time_point", [] {
        KSS_ASSERT(checkConvertTimePoint<chrono::system_clock>(20000h));
        KSS_ASSERT(checkConvertTimePoint<chrono::system_clock>(1200000min));
        KSS_ASSERT(checkConvertTimePoint<chrono::system_clock>(72000000s));
        KSS_ASSERT(checkConvertTimePoint<chrono::system_clock>(72000000000ms));
        KSS_ASSERT(checkConvertTimePoint<chrono::system_clock>(72000000000000us));
        KSS_ASSERT(checkConvertTimePoint<chrono::system_clock>(72000000000000000ns));

        KSS_ASSERT(checkConvertTimePoint<chrono::steady_clock>(20000h));
        KSS_ASSERT(checkConvertTimePoint<chrono::steady_clock>(1200000min));
        KSS_ASSERT(checkConvertTimePoint<chrono::steady_clock>(72000000s));
        KSS_ASSERT(checkConvertTimePoint<chrono::steady_clock>(72000000000ms));
        KSS_ASSERT(checkConvertTimePoint<chrono::steady_clock>(72000000000000us));
        KSS_ASSERT(checkConvertTimePoint<chrono::steady_clock>(72000000000000000ns));
    })
});
