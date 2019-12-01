//
//  nicenumber.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-03-12.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/util/nicenumber.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::util;
using namespace kss::test;


static TestSuite ts("::nicenumber", {
make_pair("niceNumber", [] {
    KSS_ASSERT(niceNumber(7.23e-7) == 1e-6);
    KSS_ASSERT(niceNumber(0.1) == 0.1);
    KSS_ASSERT(niceNumber(0.6) == 1.);
    KSS_ASSERT(niceNumber(1.1234) == 1);
    KSS_ASSERT(niceNumber(123) == 100);
    KSS_ASSERT(niceNumber(7632) == 10000);
    KSS_ASSERT(niceNumber(3827347.843) == 5e+6);
    KSS_ASSERT(niceNumber(1.234e7) == 1e+7);
})
});
