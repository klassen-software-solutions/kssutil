//
//  raii.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2016-08-26.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//
// 	Permission is hereby granted to use, modify, and publish this file without restriction other
// 	than to recognize that others are allowed to do the same.
//

#include <kss/util/raii.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::util;
using namespace kss::test;

static TestSuite ts("::raii", {
    make_pair("RAII", [] {
        bool wasSetup = false;
        bool wasCleanedUp = false;
        {
            RAII testCleaner([&]{ wasSetup = true; }, [&]{ wasCleanedUp = true; });
            KSS_ASSERT(wasSetup == true && wasCleanedUp == false);
        }
        KSS_ASSERT(wasSetup == true && wasCleanedUp == true);
    }),
    make_pair("finally", [] {
        bool wasCleanedUp = false;
        {
            finally testCleaner([&]{ wasCleanedUp = true; });
            KSS_ASSERT(wasCleanedUp == false);
        }
        KSS_ASSERT(wasCleanedUp == true);
    })
});
