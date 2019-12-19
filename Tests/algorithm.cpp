//
//  algorithm.cpp
//  KSSCore
//
//  Created by Steven W. Klassen on 2014-12-27.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cstdlib>
#include <vector>

#include <kss/test/all.h>
#include <kss/util/algorithm.hpp>

using namespace std;
using namespace kss::util;
using namespace kss::test;

namespace {

    class Counter {
    public:
        Counter(int reference) {
            _count = 0;
            _reference = reference;
        }
        Counter(const Counter& c) {
            _count = c._count;
            _reference = c._reference;
        }

        void increment() { ++_count; }
        void reset() { _count = 0; }
        int reference() const { return _reference; }
        int count() const { return _count; }

    private:
        int _count;
        int _reference;
    };

    auto incr = [](Counter& c) { c.increment(); };
    auto iseven = [](const Counter& c) { return ((c.reference() % 2) == 0); };
    auto isdiv5 = [](const Counter& c){ return ((c.reference() % 5) == 0); };
}


static TestSuite ts("::algorithm", {
    make_pair("forEachIf", [] {
        Counter counters[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

        forEachIf(counters, counters+10, incr, iseven);
        KSS_ASSERT(counters[0].count() == 0
                   && counters[2].count() == 0
                   && counters[4].count() == 0
                   && counters[6].count() == 0
                   && counters[8].count() == 0);
        KSS_ASSERT(counters[1].count() == 1
                   && counters[3].count() == 1
                   && counters[5].count() == 1
                   && counters[7].count() == 1
                   && counters[9].count() == 1);

        auto op = forEachIf(counters, counters+10, incr, isdiv5);
        KSS_ASSERT(counters[0].count() == 0
                   && counters[2].count() == 0
                   && counters[6].count() == 0
                   && counters[8].count() == 0);
        KSS_ASSERT(counters[1].count() == 1
                   && counters[3].count() == 1
                   && counters[5].count() == 1
                   && counters[7].count() == 1);
        KSS_ASSERT(counters[4].count() == 1);
        KSS_ASSERT(counters[9].count() == 2);
        KSS_ASSERT(op == incr);
    }),
    make_pair("notEqual", [] {
        int ar1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        int ar2[] = { 1, 2, -3, 4, 5, 6, 7, 8, 9, 10 };
        int ar3[] = { 0, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        int ar4[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
        int ar5[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        vector<int> v2 { 1, 2, -3, 4, 5, 6, 7, 8, 9, 10 };
        vector<int> v3 { 0, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        vector<int> v4 = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
        vector<int> v5 = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        auto absEq = [](int a, int b) { return (abs(a) == abs(b)); };

        // Simple comparisons.
        KSS_ASSERT(notEqual(ar1, ar1+10, ar2));
        KSS_ASSERT(notEqual(ar1, ar1+10, ar3));
        KSS_ASSERT(notEqual(ar1, ar1+10, ar4));
        KSS_ASSERT(!notEqual(ar1, ar1+10, ar1));
        KSS_ASSERT(!notEqual(ar1, ar1+10, ar5));

        // Comparisons with an explicit operator.
        KSS_ASSERT(!notEqual(ar1, ar1+10, ar2, absEq));
        KSS_ASSERT(notEqual(ar1, ar1+10, ar3, absEq));
        KSS_ASSERT(notEqual(ar1, ar1+10, ar4, absEq));
        KSS_ASSERT(!notEqual(ar1, ar1+10, ar1, absEq));
        KSS_ASSERT(!notEqual(ar1, ar1+10, ar5, absEq));

        // Comparisons between iterator types.
        KSS_ASSERT(notEqual(ar1, ar1+10, v2.begin()));
        KSS_ASSERT(notEqual(ar1, ar1+10, v3.begin()));
        KSS_ASSERT(notEqual(ar1, ar1+10, v4.begin()));
        KSS_ASSERT(!notEqual(ar1, ar1+10, v5.begin()));
        KSS_ASSERT(!notEqual(ar1, ar1+10, v2.begin(), absEq));
        KSS_ASSERT(notEqual(ar1, ar1+10, v3.begin(), absEq));
    })
});
