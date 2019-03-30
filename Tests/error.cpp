//
// FILENAME:    error.cpp
// AUTHOR:      Steven Klassen
// CREATED ON:  2011-08-01
//
// DESCRIPTION: Error handling in the KSS library.
//
// This file is Copyright (c) 2011 by Klassen Software Solutions. All rights reserved.
// Licensing follows the MIT License.
//

#include <iostream>
#include <stdexcept>
#include <vector>

#include <kss/util/error.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::util;
using namespace kss::test;

static TestSuite ts("::error", {
    make_pair("tryAll", [] {
        KSS_ASSERT(isFalse([] {
            return tryAll([]{ throw length_error("just some exception"); });
        }));

        KSS_ASSERT(isTrue([] {
            return tryAll([]{ });
        }));

        KSS_ASSERT(isTrue([] {
            auto p = tryAll<vector<int>>([] {
                return vector<int> { 1, 2, 3 };
            });
            return (p.second == true && p.first.size() == 3);
        }));

        KSS_ASSERT(isTrue([] {
            auto p = tryAll<vector<int>>([] {
                throw length_error("just some exception");
                return vector<int> { 1, 2, 3 };
            });
            return (p.second == false && p.first.empty());
        }));
    }),
    make_pair("errorDescription", [] {
        runtime_error e1("this is a test");
        string s = errorDescription(e1);
        KSS_ASSERT(s.find("runtime_error") != string::npos);
        KSS_ASSERT(s.find("this is a test") != string::npos);

        system_error e2(ENOMEM, system_category());
        s = errorDescription(e2);
        KSS_ASSERT(s.find("system_error") != string::npos);
        KSS_ASSERT(s.find(to_string(ENOMEM)) != string::npos);
        KSS_ASSERT(s.find(strerror(ENOMEM)) != string::npos);

        system_error e3(ENOMEM, system_category(), "mywhat");
        s = errorDescription(e3);
        KSS_ASSERT(s.find("system_error") != string::npos);
        KSS_ASSERT(s.find(to_string(ENOMEM)) != string::npos);
        KSS_ASSERT(s.find(strerror(ENOMEM)) != string::npos);
        KSS_ASSERT(s.find("mywhat") != string::npos);
    })
});
