//
//  timeutil.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-01-04.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

/*!
 \file
 \brief Miscellaneous algorithms related to time_point and duration objects.
 */

#ifndef kssutil_timeutil_hpp
#define kssutil_timeutil_hpp

#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <istream>
#include <locale>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <kss/contract/all.h>

namespace kss { namespace util { namespace time {

    /*!
     A checked version of duration_cast. Note that this will be less efficient
     than std::chrono::duration_cast. In particular it is not a constexpr plus
     it needs to perform checks before the cast can be made.

     This is based on code found at
     https://stackoverflow.com/questions/44635941/how-to-check-for-overflow-in-duration-cast?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa

     @throws std::overflow_error if the cast would not be representable in the target duration.
     */
    template <class ToDuration, class Rep, class Period>
    ToDuration checkedDurationCast(const std::chrono::duration<Rep, Period>& dtn) {
        using namespace std::chrono;
        using S = duration<long double, typename ToDuration::period>;
        constexpr S minimumAllowed = ToDuration::min();
        constexpr S maximumAllowed = ToDuration::max();
        const S s = dtn;
        if (s < minimumAllowed || s > maximumAllowed) {
            throw std::overflow_error("checked_duration_cast");
        }
        return duration_cast<ToDuration>(s);
    }

    namespace _private {
        std::string format(const std::string &fmt, const struct tm &tm) noexcept;

        struct tm& parseIso8601(const std::string& timestr,
                                struct tm& tm,
                                std::chrono::nanoseconds& ns);

        std::string toIso8601(const struct tm& tm,
                              const std::chrono::nanoseconds& ns);

        time_t parseLocalized(const std::string& s,
                              const std::locale& loc,
                              const std::string& tzone);

        std::string toLocalized(time_t t,
                                const std::locale& loc,
                                const std::string& tzone);

        time_t readFromInputStream(std::istream& strm);
        time_t tmToTimeT(const struct tm* tm);
        struct tm* tzTimeR(const time_t* timep, struct tm* result, const char* tzone, char** tzout);
    }

    /*!
     Format and parse times in the format described for ISO 8601. The parsing returns
     a reference to its tm argument. The formatting always returns the time in Zulu.

     Limitations in the parsing:
     - does not support short forms (e.g. must be 2017-08-07 and not 20170807)
     - does not support week dates (e.g. cannot use 2017-W32-1)
     - does not support ordinal dates (e.g. cannot use 2017-219)
     - if you use reduced accuracy (e.g. 2017 or 2017-08-07T12) the missing fields
        will be midnight, January 1st.
     - you cannot leave out omit larger items (e.g. cannot use --08-07 to leave out
        the year)
     - you can only have decimals in the seconds
     - decimal seconds is only supported in the form that takes a duration argument.

     @throws std::system_error if the string could not be parsed.
     @throws std::invalid_argument if the timestr is empty.
     @throws kss::util::time::checkedDuratonCast if the duration version could not
        convert from nanoseconds to the desired duration.
     */
    inline std::string formatIso8601(const struct tm& tm) noexcept {
        return _private::format("%FT%TZ", tm);
    }
    inline struct tm& parseIso8601(const std::string& timestr, struct tm& tm) {
        std::chrono::nanoseconds ns;
        return _private::parseIso8601(timestr, tm, ns);
    }

    template <class Duration>
    inline struct tm& parseIso8601(const std::string& timestr, struct tm& tm, Duration& subseconds) {
        std::chrono::nanoseconds ns;
        _private::parseIso8601(timestr, tm, ns);
        subseconds = checkedDurationCast<Duration>(ns);
        return tm;
    }

    /*!
     Returns the time (real time) taken to execute the given block.
     */
    std::chrono::milliseconds timeOfExecution(const std::function<void()>& fn);


    // MARK: time_point extensions

    // Note that the TimePoint template type needs to be a type compatible with
    // std::chrono::time_point<Clock, Duration> for some value of Clock and
    // Duration. This is checked using the following macro. If you get errors
    // reported on this macro, check that your argument for TimePoint is
    // actually a time_point type.
#   define _KSS_IS_TIMEPOINT(TP) \
        static_assert(std::is_object<typename TP::clock>::value \
            && std::is_object<typename TP::duration>::value, \
            "TP must be a std::chrono::time_point<Clock, Duration>")

    /*!
     Returns a time_point constructed to contain the current time as defined by
     the time point's clock.

     @throws kss::util::time::checkedDurationCast if the duration value for
        TimePoint::clock::time_point::duration cannot be represented as a duration
        value for TimePoint::duration.
     */
    template <class TimePoint>
    inline TimePoint now(const TimePoint& = TimePoint()) {
        _KSS_IS_TIMEPOINT(TimePoint);
        using Clock = typename TimePoint::clock;
        using Duration = typename TimePoint::duration;
        const auto dur = Clock::now().time_since_epoch();
        return TimePoint(checkedDurationCast<Duration>(dur));
    }

    /*!
     Returns a time_point constructed from a time_t value.

     @throws std::overflow_error if the underlying time point representation cannot
        handle the value of t seconds.
     */
    template <class TimePoint>
    inline TimePoint fromTimeT(time_t t, const TimePoint& = TimePoint()) {
        _KSS_IS_TIMEPOINT(TimePoint);
        const auto secs = std::chrono::seconds(t);
        return TimePoint(checkedDurationCast<typename TimePoint::duration>(secs));
    }

    /*!
     Returns a time_point constructed from a C tm structure.
     */
    template <class TimePoint>
    inline TimePoint fromTm(const struct tm& tm, const TimePoint& typeArg = TimePoint()) {
        _KSS_IS_TIMEPOINT(TimePoint);
        return fromTimeT(_private::tmToTimeT(&tm), typeArg);
    }

    /*!
     Returns a time_point from an ISO8601 string. Note the following caveats:
     - this has at best nanosecond accuracy
     - the accuracy will be further restricted by the type of Duration
     (i.e. if you read sub-second values into a time_point that only has
     second accuracy, the sub-second portion of the string will be lost).

     @throws std::system_error if the string could not be parsed.
     */
    template <class TimePoint>
    TimePoint fromIso8601String(const std::string& s, const TimePoint& typeArg = TimePoint()) {
        _KSS_IS_TIMEPOINT(TimePoint);
        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        std::chrono::nanoseconds ns(0);
        parseIso8601(s, tm, ns);

        auto t = fromTm(tm, typeArg);
        if (ns.count() != 0) {
            t += std::chrono::duration_cast<typename TimePoint::duration>(ns);
        }
        return t;
    }

    /*!
     Obtain a timestamp by parsing a string in the given locale. Note that
     this will not handle the fractional portion of seconds.

     @param s the string to parse
     @param loc the locale that the string representation should be using. Assumes
        the current global locale if not specified.
     @param tzone the timezone the string representation is using. Assumes
        the local time zone of the machine if not specified.
     @param typeArg ignored, used to force the template type
     @throws std::system_error if there is a problem parsing.
     @throws std::invalid_argument if the time zone information cannot be found
        for the given time zone string.
     @throws std::invalid_argument if s is empty
     */
    template <class TimePoint>
    inline TimePoint fromLocalizedString(const std::string& s,
                                         const std::locale& loc = std::locale(),
                                         const std::string& tzone = std::string(),
                                         const TimePoint& typeArg = TimePoint())
    {
        _KSS_IS_TIMEPOINT(TimePoint);
        return fromTimeT(_private::parseLocalized(s, loc, tzone), typeArg);
    }

    /*!
     Convert a time point to a time_t object. Note that this will lose any
     sub-second accuracy that the time point may include.
     @throws std::overflow_error if the time point cannot be represented by a time_t.
     */
    template <class Clock, class Duration = typename Clock::duration>
    time_t toTimeT(const std::chrono::time_point<Clock, Duration>& tp) {
        using ttduration = std::chrono::duration<time_t, std::chrono::seconds::period>;
        const auto d = checkedDurationCast<ttduration>(tp.time_since_epoch());
        return d.count();
    }

    /*!
     Convert a time point to a struct tm C object. Note that this will lose any
     sub-second accuracy that the time point may include.
     @throws std::overflow_error if the time point cannot be represented by a time_t
     (which is used internally in the conversion to a struct tm).
     */
    template <class Clock, class Duration = typename Clock::duration>
    struct tm& toTm(const std::chrono::time_point<Clock, Duration>& tp, struct tm& tm) {
        const auto tt = toTimeT(tp);
        _private::tzTimeR(&tt, &tm, nullptr, nullptr);
        return tm;
    }

    /*!
     Convert a time point to an ISO8601 string. Note that this implementation always
     returns the string in the zulu time zone.

     @throws std::overflow_error if the time point cannot be represented in seconds
     @throws any exception that string creation may throw.
     */
    template <class Clock, class Duration = typename Clock::duration>
    std::string toIso8601String(const std::chrono::time_point<Clock, Duration>& tp) {
        struct tm tm;
        toTm(tp, tm);
        const auto secs = checkedDurationCast<std::chrono::seconds>(tp.time_since_epoch());
        const auto subsecs = tp.time_since_epoch() - secs;
        std::chrono::nanoseconds ns(0);
        if (subsecs.count() > 0) {
            typename Duration::period p;
            const auto subSecsAsSeconds = (long double)subsecs.count() * p.num / p.den;
            kss::contract::conditions({
                KSS_EXPR(subSecsAsSeconds <= 1) // should be a fraction of a second
            });
            const auto subSecsRoundedToNs = std::round(subSecsAsSeconds * 1000000000);
            const auto rep = std::chrono::nanoseconds::rep(subSecsRoundedToNs);
            ns = std::chrono::nanoseconds(rep);
        }
        return _private::toIso8601(tm, ns);
    }

    /*!
     Convert a time point to a localized string.

     @param tp the time point to convert
     @param loc the locale that defines the string format (assumes the global locale
     if not specified)
     @param tzone the timezone the value should be shown in (assumes the local
     time zone if not specified)
     @throws std::invalid_argument if the time zone information cannot be found
     for the given time zone string
     */
    template <class Clock, class Duration = typename Clock::duration>
    inline std::string toLocalizedString(const std::chrono::time_point<Clock, Duration>& tp,
                                         const std::locale& loc = std::locale(),
                                         const std::string& tzone = std::string())
    {
        return _private::toLocalized(toTimeT(tp), loc, tzone);
    }

}}}

namespace std {
    /*!
     Write a time point to an output stream. This will output a localized value
     using the current locale of the stream.
     */
    template <class Clock, class Duration = typename Clock::duration>
    std::ostream& operator<<(std::ostream& strm,
                             const std::chrono::time_point<Clock, Duration>& tp)
    {
        const auto loc = strm.getloc();
        strm << kss::util::time::toLocalizedString(tp, loc);
        return strm;
    }

    /*!
     Read a time point from an input stream. This will read a localized value
     using the current locale of the stream.
     */
    template <class Clock, class Duration = typename Clock::duration>
    inline std::istream& operator>>(std::istream& strm,
                                    std::chrono::time_point<Clock, Duration>& tp)
    {
        tp = kss::util::time::fromTimeT(kss::util::time::_private::readFromInputStream(strm), tp);
        return strm;
    }
}
#endif
