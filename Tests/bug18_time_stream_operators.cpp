//
//  bug18_time_stream_operators.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-12-27.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#include <sstream>

#include <kss/test/all.h>
#include <kss/util/timeutil.hpp>

using namespace std;
using namespace kss::util;
using namespace kss::test;

using time_point_t = chrono::time_point<chrono::steady_clock, chrono::milliseconds>;

void bug18_stream_operators() {
    // Note that to actually test the bug this needs to be compiled separately from
    // the rest of the timeutil tests. Specifically we need that kss::util::time is not
    // automatically included.
    KSS_ASSERT(doesNotThrowException([] {
        std::ostringstream strm;
        const auto tp = time::now<time_point_t>();
        strm << "expected a value less than (" << tp << ")";
    }));
    KSS_ASSERT(doesNotThrowException([] {
        std::istringstream strm("2018-04-13T08:15:05Z");
        time_point_t tp;
        strm >> tp;
    }));
}
