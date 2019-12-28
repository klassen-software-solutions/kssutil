//
//  timeutil.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-01-04.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <string>
#include <thread>

#include <kss/test/all.h>
#include <kss/util/timeutil.hpp>

using namespace std;
using namespace kss::util;
using namespace kss::util::time;
using namespace kss::test;

extern void bug18_stream_operators();

namespace {
	bool checkTm(const struct ::tm& tm, int year, int month, int day, int hour, int minute, int second) noexcept {
		return tm.tm_year == year - 1900
			&& tm.tm_mon == month - 1
			&& tm.tm_mday == day
			&& tm.tm_hour == hour
			&& tm.tm_min == minute
			&& tm.tm_sec == second;
	}

	template <class Duration>
	bool checkTmD(const struct ::tm& tm, int year, int month, int day, int hour, int minute, int second,
				  Duration& d, const Duration& dexpected) noexcept
	{
		return checkTm(tm, year, month, day, hour, minute, second)
			&& (d == dexpected);
	}
}

static TestSuite ts("time::timeutil", {
    make_pair("Iso8601", [] {
        // formatting
        struct ::tm tm;
        memset(&tm, 0, sizeof(struct ::tm));
        tm.tm_year = 117;
        tm.tm_mon = 7;
        tm.tm_mday = 7;
        tm.tm_hour = 13;
        tm.tm_min = 53;
        tm.tm_sec = 2;
        KSS_ASSERT(formatIso8601(tm) == "2017-08-07T13:53:02Z");

        tm.tm_year = -80;
        KSS_ASSERT(formatIso8601(tm) == "1820-08-07T13:53:02Z");

        // parsing
        KSS_ASSERT(checkTm(parseIso8601("2017", tm), 2017, 1, 1, 0, 0, 0));
        KSS_ASSERT(checkTm(parseIso8601("2017-08", tm), 2017, 8, 1, 0, 0, 0));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07", tm), 2017, 8, 7, 0, 0, 0));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T12", tm), 2017, 8, 7, 12, 0, 0));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T12:01", tm), 2017, 8, 7, 12, 1, 0));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T12:01:02", tm), 2017, 8, 7, 12, 1, 2));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T12:01:02Z", tm), 2017, 8, 7, 12, 1, 2));

        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T12:01:02+00", tm), 2017, 8, 7, 12, 1, 2));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T12:01:02+0000", tm), 2017, 8, 7, 12, 1, 2));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T12:01:02+00:00", tm), 2017, 8, 7, 12, 1, 2));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T23:58:05-0330", tm), 2017, 8, 8, 3, 28, 5));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T23:58:05-03:30", tm), 2017, 8, 8, 3, 28, 5));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T23:58:00-03:30", tm), 2017, 8, 8, 3, 28, 0));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T23:58:05+02", tm), 2017, 8, 7, 21, 58, 5));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T23:58:05+0200", tm), 2017, 8, 7, 21, 58, 5));
        KSS_ASSERT(checkTm(parseIso8601("2017-08-07T23:58:05+02:00", tm), 2017, 8, 7, 21, 58, 5));

        chrono::milliseconds ms;
        const chrono::milliseconds msexpected(1);
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T11:53:10.001", tm, ms), 2017, 8, 7, 11, 53, 10, ms, msexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T11:53:10.001Z", tm, ms), 2017, 8, 7, 11, 53, 10, ms, msexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.001+02", tm, ms), 2017, 8, 7, 11, 53, 10, ms, msexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.001+0200", tm, ms), 2017, 8, 7, 11, 53, 10, ms, msexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.001+02:00", tm, ms), 2017, 8, 7, 11, 53, 10, ms, msexpected));

        chrono::microseconds us;
        const chrono::microseconds usexpected(1);
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T11:53:10.000001", tm, us), 2017, 8, 7, 11, 53, 10, us, usexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T11:53:10.000001Z", tm, us), 2017, 8, 7, 11, 53, 10, us, usexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.000001+02", tm, us), 2017, 8, 7, 11, 53, 10, us, usexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.000001+0200", tm, us), 2017, 8, 7, 11, 53, 10, us, usexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.000001+02:00", tm, us), 2017, 8, 7, 11, 53, 10, us, usexpected));

        chrono::nanoseconds ns;
        const chrono::nanoseconds nsexpected(1);
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T11:53:10.000000001", tm, ns), 2017, 8, 7, 11, 53, 10, ns, nsexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T11:53:10.000000001Z", tm, ns), 2017, 8, 7, 11, 53, 10, ns, nsexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.000000001+02", tm, ns), 2017, 8, 7, 11, 53, 10, ns, nsexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.000000001+0200", tm, ns), 2017, 8, 7, 11, 53, 10, ns, nsexpected));
        KSS_ASSERT(checkTmD(parseIso8601("2017-08-07T13:53:10.000000001+02:00", tm, ns), 2017, 8, 7, 11, 53, 10, ns, nsexpected));

        KSS_ASSERT(throwsException<system_error>([&] { parseIso8601("201708", tm); }));
        KSS_ASSERT(throwsException<system_error>([&] { parseIso8601("20170807", tm); }));
        KSS_ASSERT(throwsException<system_error>([&] { parseIso8601("2017-08-07 12:01:02", tm); }));
        KSS_ASSERT(throwsException<system_error>([&] { parseIso8601("2017-08-07T120102", tm); }));
        KSS_ASSERT(throwsException<system_error>([&] { parseIso8601("helloworldT12:01:02", tm); }));
        KSS_ASSERT(throwsException<system_error>([&] { parseIso8601("2017-08-07T12:00:00+25", tm); }));
        KSS_ASSERT(throwsException<system_error>([&] { parseIso8601("2017-08-07T12:00:00-25", tm); }));
    }),
    make_pair("checkedDurationCast", [] {
        using namespace std::chrono;

        auto dtn = chrono::minutes::max() - chrono::minutes(10);
        KSS_ASSERT(throwsException<overflow_error>([&] {
            checkedDurationCast<chrono::microseconds>(dtn);
        }));
        KSS_ASSERT(doesNotThrowException([&] {
            checkedDurationCast<chrono::hours>(dtn);
        }));

        dtn = chrono::minutes::min() + chrono::minutes(10);
        KSS_ASSERT(throwsException<overflow_error>([&] {
            checkedDurationCast<chrono::microseconds>(dtn);
        }));
        KSS_ASSERT(doesNotThrowException([&] {
            checkedDurationCast<chrono::hours>(dtn);
        }));

        dtn = chrono::minutes(10);
        KSS_ASSERT(doesNotThrowException([&] {
            checkedDurationCast<chrono::hours>(dtn);
        }));

        const seconds bigSeconds(numeric_limits<seconds::rep>::max());
        const milliseconds millis = duration_cast<milliseconds>(bigSeconds);
        KSS_ASSERT(millis.count() < bigSeconds.count());    // had an overflow
        KSS_ASSERT(throwsException<overflow_error>([&] {
            checkedDurationCast<milliseconds>(bigSeconds);
        }));
    }),
    make_pair("timeOfExecution", [] {
        KSS_ASSERT(isTrue([] {
            const auto t = timeOfExecution([]{
                this_thread::sleep_for(chrono::microseconds(2000));
            });
            return (t.count() > 1);
        }));
    }),
    make_pair("time_point factory methods", [] {
        using namespace std::chrono;
        using timestamp_s = time_point<system_clock>;
        using timestamp_ms = time_point<system_clock, milliseconds>;
        using timestamp_us = time_point<system_clock, microseconds>;
        using timestamp_ns = time_point<system_clock, nanoseconds>;
        const auto base = 20000h + 15min + 5s;

        KSS_ASSERT(fromTimeT<timestamp_s>(time_t(20000*60*60 + 15*60 + 5)).time_since_epoch() == base);

        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        tm.tm_year = 72;
        tm.tm_mon = 3;
        tm.tm_mday = 13;
        tm.tm_hour = 8;
        tm.tm_min = 15;
        tm.tm_sec = 5;
        KSS_ASSERT(fromTm<timestamp_s>(tm).time_since_epoch() == base);

        KSS_ASSERT(fromIso8601String<timestamp_s>("1972-04-13T08:15:05Z").time_since_epoch() == base);
        KSS_ASSERT(fromIso8601String<timestamp_ms>("1972-04-13T08:15:05.001Z").time_since_epoch() == (base + 1ms));
        KSS_ASSERT(fromIso8601String<timestamp_us>("1972-04-13T08:15:05.000001Z").time_since_epoch() == (base + 1us));
        KSS_ASSERT(fromIso8601String<timestamp_ns>("1972-04-13T08:15:05.000000001Z").time_since_epoch() == (base + 1ns));

#if !defined(__linux)
        // cannot seem to get linux to accept the locales

        KSS_ASSERT(fromLocalizedString<timestamp_s>("Thu 13 Apr 08:15:05 1972",
                                                    locale("en_CA"),
                                                    "GMT"
                                                    ).time_since_epoch() == base);
        KSS_ASSERT(fromLocalizedString<timestamp_s>("Thu 13 Apr 01:15:05 1972",
                                                    locale("en_CA"),
                                                    "America/Edmonton"
                                                    ).time_since_epoch() == base);
        KSS_ASSERT(fromLocalizedString<timestamp_s>("Do 13 Apr 09:15:05 1972",
                                                    locale("de_DE"),
                                                    "Europe/Vienna"
                                                    ).time_since_epoch() == base);
#endif
    }),
    make_pair("time_point to...", [] {
        using namespace std::chrono;
        using timestamp_ns = time_point<system_clock, nanoseconds>;
        const auto t = fromIso8601String<timestamp_ns>("1972-04-13T08:15:05Z");
        KSS_ASSERT(toTimeT(t) == time_t(20000*60*60 + 15*60 + 5));

        struct tm tm;
        toTm(t, tm);
        KSS_ASSERT(tm.tm_year == 72 && tm.tm_mon == 3 && tm.tm_mday == 13
                   && tm.tm_hour == 8 && tm.tm_min == 15 && tm.tm_sec == 5);

        KSS_ASSERT(toIso8601String(t) == "1972-04-13T08:15:05Z");
        KSS_ASSERT(toIso8601String(t + 1ms) == "1972-04-13T08:15:05.001Z");
        KSS_ASSERT(toIso8601String(t + 1us) == "1972-04-13T08:15:05.000001Z");
        KSS_ASSERT(toIso8601String(t + 1ns) == "1972-04-13T08:15:05.000000001Z");

        // Don't know what locale it is, but should produce something.
        KSS_ASSERT(!toLocalizedString(t).empty());

#if !defined(__linux)
        // cannot seem to get linux to accept the locales

        // Don't know what time zone to use, but should produce something.
        KSS_ASSERT(!toLocalizedString(t, locale("en_CA")).empty());
        KSS_ASSERT(!toLocalizedString(t, locale("de_DE")).empty());

        KSS_ASSERT(toLocalizedString(t, locale("en_CA"), "GMT") == "Thu 13 Apr 08:15:05 1972");
        KSS_ASSERT(toLocalizedString(t, locale("en_CA"), "America/Edmonton") == "Thu 13 Apr 01:15:05 1972");
        KSS_ASSERT(toLocalizedString(t, locale("de_DE"), "Europe/Vienna") == "Do 13 Apr 09:15:05 1972");
#endif
    }),
#if !defined(__linux)
    // cannot seem to get linux to accept the locales

    make_pair("time_point I/O streams", [] {
        using timestamp_s = chrono::time_point<chrono::system_clock>;
        const timestamp_s t(20000h + 15min + 5s);

        // Stream output. We don't know the time zone we are in, so all we can check
        // at this point is that the output has written something.
        ostringstream strm;
        strm.imbue(locale::classic());
        strm << t << endl;
        strm.imbue(locale("de_DE"));
        strm << t;
        KSS_ASSERT(!strm.str().empty());

        // Stream input. At this point we check that when we read in the output we just
        // wrote, we get the same time point values.
        timestamp_s tmp;
        istringstream istrm(strm.str());
        istrm.imbue(locale::classic());
        istrm >> tmp;
        KSS_ASSERT(tmp == t);

        tmp = timestamp_s();
        istrm.imbue(locale("de_DE"));
        istrm >> skipws >> tmp;
        KSS_ASSERT(tmp == t);
    }),
#endif
    make_pair("time_point now", [] {
        using timestamp_s = chrono::time_point<chrono::system_clock>;
        const auto t = fromIso8601String<timestamp_s>("2018-04-13T08:15:05Z");
        const timestamp_s ts = now<timestamp_s>();
        KSS_ASSERT(ts > t);

        KSS_ASSERT(isTrue([&] {
            using fractional_seconds_t = chrono::duration<long double, std::chrono::seconds::period>;
            using timestamp_fs = chrono::time_point<chrono::system_clock, fractional_seconds_t>;
            const timestamp_fs fts = now<timestamp_fs>();
            return (fts > t);
        }));
    }),
    make_pair("time_point floating precision", [] {
        // This test covers a precision related bug found when using a fractional second
        // timestamps at Frauscher. The problem was that for some timestamp values the
        // value written, then read back would differ from the original by a few ns.
        using timeinterval_t = std::chrono::duration<long double, std::chrono::seconds::period>;
        using timestamp_t = std::chrono::time_point<std::chrono::system_clock, timeinterval_t>;
        auto durationSinceEpoch = timeinterval_t(1534776783.41737399995L);
        auto ts = timestamp_t(durationSinceEpoch);
        auto tsAsString = toIso8601String(ts);
        auto ts2 = fromIso8601String<timestamp_t>(tsAsString);
        KSS_ASSERT(ts == ts2);

        durationSinceEpoch = timeinterval_t(1534785685.52626399999L);
        ts = timestamp_t(durationSinceEpoch);
        tsAsString = toIso8601String(ts);
        ts2 = fromIso8601String<timestamp_t>(tsAsString);
        KSS_ASSERT(ts == ts2);

        durationSinceEpoch = timeinterval_t(1534796154.17710480001L);
        ts = timestamp_t(durationSinceEpoch);
        tsAsString = toIso8601String(ts);
        ts2 = fromIso8601String<timestamp_t>(tsAsString);
        KSS_ASSERT(ts == ts2);
    }),
    make_pair("bug18 stream operators", bug18_stream_operators)
});
