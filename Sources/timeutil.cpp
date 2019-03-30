//
//  timeutil_cpp.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-01-04.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>
#include <sstream>

#include "_contract.hpp"
#include "raii.hpp"
#include "stringutil.hpp"
#include "timeutil.hpp"

using namespace std;
using namespace kss::util;
using namespace kss::util::time;
namespace contract = kss::util::_private::contract;


namespace {
    // Format and parse times.
    bool ftime_with_buffer(const string& format, const tm& tm, string& ret, size_t bufsiz) {
        char buffer[bufsiz+1];
        if (!strftime(buffer, bufsiz, format.c_str(), &tm)) {
            return false;
        }
        else {
            ret = string(buffer);
            return true;
        }
    }
}

string kss::util::time::_private::format(const std::string &fmt, const struct tm &tm) noexcept {
    assert(!fmt.empty());
    size_t bufsiz = fmt.size() + 100;
    string ret;
    while (!ftime_with_buffer(fmt, tm, ret, bufsiz)) {
        bufsiz += (bufsiz / 2);
    }
    return ret;
}

time_t kss::util::time::_private::tmToTimeT(const struct tm* tm) {
    assert(tm != nullptr);

    struct tm tmtemp;
    memcpy(&tmtemp, tm, sizeof(struct tm));
    time_t t = timegm(&tmtemp);
    if (tm->tm_gmtoff)
        t = t - tm->tm_gmtoff;
    return t;
}

// Convert a time to a tm structure taking a time zone into account.
static pthread_mutex_t  ENVMUTEX = PTHREAD_MUTEX_INITIALIZER;
struct tm* kss::util::time::_private::tzTimeR(const time_t* timep,
                                              struct tm* result,
                                              const char* tzone,
                                              char** tzout)
{
    assert(timep != nullptr);
    assert(result != nullptr);
    
    errno = 0;
    if (!tzone) {
        gmtime_r(timep, result);
        if (tzout != NULL)
            *tzout = strdup("UTC");
    }
    else {
        int err = pthread_mutex_lock(&ENVMUTEX);
        if (err) {
            errno = err;
            return NULL;
        }

        char* oldtz = getenv("TZ");
        setenv("TZ", tzone, 1);
        tzset();

        if (localtime_r(timep, result) != NULL && tzout != NULL) {
            if (result->tm_zone) {
                *tzout = strdup(result->tm_zone);
            }
            else {
                *tzout = strdup("");
            }
        }

        if (oldtz) {
            setenv("TZ", oldtz, 1);
        }
        else {
            unsetenv("TZ");
        }
        tzset();

        err = pthread_mutex_unlock(&ENVMUTEX);
        if (err) {
            errno = err;
        }
    }

    return (errno != 0 ? NULL : result);
}

namespace {
	inline void throw_invalid(const string& timestr) {
		throw system_error(EINVAL, system_category(), "Could not parse '" + timestr + "'");
	}

    long to_integer(const string& s, const string& timestr) {
        char* endptr = nullptr;
        long i = strtol(s.c_str(), &endptr, 10);
        if (*endptr != '\0') { throw_invalid(timestr); }
        return i;
    }

    long double to_ldouble(const string& s, const string& timestr) {
        char* endptr = nullptr;
        const long double d = strtold(s.c_str(), &endptr);
        if (*endptr != '\0') { throw_invalid(timestr); }
        return d;
    }

    struct tm& parse(const string &timestr, const string &format, struct tm &tm) {
        assert(!timestr.empty());
        assert(!format.empty());

        if (strptime(timestr.c_str(), format.c_str(), &tm) == NULL) {
            throw_invalid(timestr);
        }
        return tm;
    }

    void parseMainPortion(const string& timestr, struct tm& tm, chrono::nanoseconds& ns,
                          bool& needToParseNanoseconds, bool& needToParseTimeZone,
                          size_t& timeZoneStart)
    {
        assert(!timestr.empty());

        string format;
        const auto len = timestr.size();
        memset(&tm, 0, sizeof(struct tm));
        tm.tm_mday = 1;
        ns = chrono::nanoseconds(0);

        if (len == 4)       { format = "%Y"; }
        else if (len == 7)  { format = "%Y-%m"; }
        else if (len == 10) { format = "%Y-%m-%d"; }
        else if (len == 13) { format = "%Y-%m-%dT%H"; }
        else if (len == 16) { format = "%Y-%m-%dT%H:%M"; }
        else if (len == 19) { format = "%Y-%m-%dT%H:%M:%S"; }
        else if (len == 20) { format = "%Y-%m-%dT%H:%M:%SZ"; }
        else if (len > 20) {
            const size_t afterFormatPos = 19;
            format = "%Y-%m-%dT%H:%M:%S";
            if (timestr[afterFormatPos] == '.') {
                needToParseNanoseconds = true;
            }
            else if (timestr[afterFormatPos] == '-'
                     || timestr[afterFormatPos] == '+'
                     || timestr[afterFormatPos] == 'Z')
            {
                needToParseTimeZone = true;
                timeZoneStart = afterFormatPos;
            }
            else {
                throw_invalid(timestr);
            }
        }
        else {
            // No action required.
        }

        if (format.empty()) {
            throw_invalid(timestr);
        }
        parse(timestr, format, tm);
    }

    chrono::nanoseconds parseNanoseconds(const string& timestr,
                                         size_t& timeZoneStart,
                                         bool& needToParseTimeZone)
    {
        assert(!timestr.empty());

        using std::chrono::nanoseconds;

        timeZoneStart = timestr.find_first_of("-+Z", 19);
        if (timeZoneStart != string::npos) {
            needToParseTimeZone = true;
        }

        const auto nsstr = timestr.substr(19, timeZoneStart-19);
        return nanoseconds(nanoseconds::rep(round(to_ldouble(nsstr, timestr) * 1000000000.L)));
    }

    void parseTimeZone(const string& timestr, struct tm& tm, size_t timeZoneStart) {
        assert(!timestr.empty());

        const auto tzstr = timestr.substr(timeZoneStart);
        const auto tzlen = tzstr.size();

        if (tzlen == 1) {
            assert(tzstr[0] == 'Z');
        }
        else {
            const string hourOffsetStr = tzstr.substr(0, 3);
            const long hourOffset = to_integer(hourOffsetStr, timestr);
            constexpr long maxHourOffset = 24;
            if (hourOffset > maxHourOffset || hourOffset < -maxHourOffset) {
                throw_invalid(timestr);
            }

            string minuteOffsetStr;
            if (tzlen == 5) {
                minuteOffsetStr = tzstr.substr(3, 2);
            }
            else if (tzlen == 6) {
                if (tzstr[3] != ':') { throw_invalid(timestr); }
                minuteOffsetStr = tzstr.substr(4, 2);
            }
            else {
                // No minute offset string needed.
            }

            const long minuteOffset = to_integer(minuteOffsetStr, timestr);
            constexpr long maximumMinuteOffset = 59L;
            if (minuteOffset < 0 || minuteOffset > maximumMinuteOffset) {
                throw_invalid(timestr);
            }

            long timeZoneShift = (abs(hourOffset) * 60L * 60L) + (minuteOffset * 60L);
            if (hourOffset > 0) { timeZoneShift *= -1; }
            if (timeZoneShift != 0L) {
                time_t t = timegm(&tm);
                t += timeZoneShift;
                gmtime_r(&t, &tm);
            }
        }
    }

    char* kss_rtrim_char(char* s, char c) {
        if (!s) {
            return nullptr;
        }
        size_t len = strlen(s);
        char* p = s + len - 1;
        while (p >= s && *p == c) {
            *p = '\0';
            --p;
        }
        return s;
    }

}

struct tm& kss::util::time::_private::parseIso8601(const string& timestr,
                                                   struct tm &tm,
                                                   chrono::nanoseconds& ns)
{
    contract::parameters({
        KSS_EXPR(!timestr.empty())
    });

    bool needToParseNanoseconds = false;
    bool needToParseTimeZone = false;
    size_t timeZoneStart = 0;
    parseMainPortion(timestr, tm, ns, needToParseNanoseconds, needToParseTimeZone, timeZoneStart);

    if (needToParseNanoseconds) {
        ns = parseNanoseconds(timestr, timeZoneStart, needToParseTimeZone);
    }

    if (needToParseTimeZone) {
        parseTimeZone(timestr, tm, timeZoneStart);
    }

    return tm;
}

string kss::util::time::_private::toIso8601(const struct tm &tm,
                                            const chrono::nanoseconds& ns)
{
    using namespace std::chrono;

    // The main date and time.
    char buffer[30];
    strftime(buffer, 20, "%Y-%m-%dT%H:%M:%S", &tm);
    ostringstream strm;
    strm << buffer;

    // The fractional portion.
    if (ns.count()) {
        const auto ms = duration_cast<milliseconds>(ns);
        if (ns == ms) {
            sprintf(buffer, ".%03ld", (long)ms.count());
        }
        else {
            const auto us = duration_cast<microseconds>(ns);
            if (ns == us) {
                sprintf(buffer, ".%06ld", (long)us.count());
            }
            else {
                sprintf(buffer, ".%09ld", (long)ns.count());
            }
        }

        kss_rtrim_char(buffer, '0');
        strm << buffer;
    }

    // The time zone.
    if (!tm.tm_gmtoff) {
        strm << "Z";
    }
    else {
        int tzhour = (int)(tm.tm_gmtoff / (60L * 60L));
        int tzminute = (int)((tm.tm_gmtoff % (60L*60L)) / 60L);
        if (tzminute < 0) {
            tzminute *= -1;
        }
        sprintf(buffer, "%+03d:%02d", tzhour, tzminute);
        strm << buffer;
    }
    return strm.str();
}

namespace {
    static mutex tzEnvMutex;

    time_t tmToTime(const struct tm& tm, const string& tzone) {
        errno = 0;
        time_t t = 0;
        struct tm tmtmp;
        memcpy(&tmtmp, &tm, sizeof(struct tm));
        if (tzone.empty()) {
            t = timelocal(&tmtmp);
            if (t == -1) {
                throw system_error(errno, system_category(), "timelocal failed");
            }
        }
        else {
            lock_guard<mutex> lock(tzEnvMutex);
            char* oldtz = getenv("TZ");

            finally cleanup([&oldtz]{
                if (oldtz) {
                    setenv("TZ", oldtz, 1);
                }
                else {
                    unsetenv("TZ");
                }
                tzset();
            });

            setenv("TZ", tzone.c_str(), 1);
            tzset();

            t = timelocal(&tmtmp);
            if (t == -1) {
                throw system_error(errno, system_category(),
                                   "timelocal failed for time zone '" + tzone + "'");
            }
        }

        return t;
    }

    void timeToTm(time_t t, struct tm& result, const string& tzone) {
        errno = 0;
        memset(&result, 0, sizeof(struct tm));
        if (tzone.empty()) {
            if (localtime_r(&t, &result) == nullptr) {
                throw system_error(errno, system_category(), "localtime_r failed");
            }
        }
        else {
            lock_guard<mutex> lock(tzEnvMutex);
            char* oldtz = getenv("TZ");

            finally cleanup([&oldtz]{
                if (oldtz) {
                    setenv("TZ", oldtz, 1);
                }
                else {
                    unsetenv("TZ");
                }
                tzset();
            });

            setenv("TZ", tzone.c_str(), 1);
            tzset();

            if (localtime_r(&t, &result) == nullptr) {
                throw system_error(errno, system_category(),
                                   "localtime_r failed for time zone '" + tzone + "'");
            }
        }
    }
}

time_t kss::util::time::_private::parseLocalized(const string& s,
                                                 const locale& loc,
                                                 const string& tzone)
{
    contract::parameters({
        KSS_EXPR(!s.empty())
    });

    const auto& tmget = use_facet<time_get<char>>(loc);
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    ios::iostate state;

    istringstream strm(s);
    const char* fmt = "%c";
    const auto len = strlen(fmt);
    tmget.get(strm, time_get<char>::iter_type(), strm, state, &tm, fmt, fmt+len);
    if (strm.bad() || strm.fail()) {
        throw system_error(EINVAL, system_category(),
                           "Could not parse '" + s + "' as a local time.");
    }
    return tmToTime(tm, tzone);
}

string kss::util::time::_private::toLocalized(time_t t,
                                              const locale& loc,
                                              const string& tzone)
{
    const auto& tmput = use_facet<time_put<char>>(loc);
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    timeToTm(t, tm, tzone);

    ostringstream strm;
    tmput.put(strm, strm, ' ', &tm, 'c');
    if (strm.bad() || strm.fail()) {
        throw system_error(EINVAL, system_category(),
                           "Could not write time point as a local time.");
    }
    return strm.str();
}

time_t kss::util::time::_private::readFromInputStream(istream& strm) {
    const auto& tmget = use_facet<time_get<char>>(strm.getloc());
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    ios::iostate state = ios::goodbit;

    if (strm.flags() & ios::skipws) {
        while (isspace(strm.peek()) && !strm.eof()) { strm.get(); }
    }

    const char* fmt = "%c";
    const auto len = strlen(fmt);
    tmget.get(strm, time_get<char>::iter_type(), strm, state, &tm, fmt, fmt+len);
    if (!(state & ios::failbit) && !(state & ios::badbit)) {
        return tmToTime(tm, "");
    }
    return (time_t)-1;
}

chrono::milliseconds kss::util::time::timeOfExecution(const function<void ()>& fn) {
    using namespace std::chrono;

    const auto start = high_resolution_clock::now();
    fn();
    return duration_cast<milliseconds>(high_resolution_clock::now() - start);
}
