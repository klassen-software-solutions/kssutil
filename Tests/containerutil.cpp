//
//  containerutil.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-05-18.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include <valarray>
#include <vector>

#include <kss/util/containerutil.hpp>

#include "ksstest.hpp"


using namespace std;
using namespace kss::util;
using namespace kss::util::containers;
using namespace kss::test;

namespace {
	int findInIterator(typename vector<int>::const_iterator it) {
		return *it;
	}

    template <class Vector>
    bool applyToVectorLikeContainer() {
        Vector vec(100);
        apply(vec, [](size_t i, const int&) {
            return 2 * int(i);
        });

        const size_t len = vec.size();
        for (size_t i = 0; i < len; ++i) {
            if (vec[i] != (2 * int(i))) {
                return false;
            }
        }

        apply(vec, [](size_t i, const int& val) {
            return val - int(i);
        });

        for (size_t i = 0; i < len; ++i) {
            if (vec[i] != int(i)) {
                return false;
            }
        }
        
        return true;
    }
}

static TestSuite ts("containers::containerutil", {
    make_pair("contains", [] {
        map<string, int> tmap;
        set<string> tset;

        tmap.insert(make_pair("one", 1));
        tmap.insert(make_pair("two", 2));
        tmap.insert(make_pair("three", 3));
        tmap.insert(make_pair("four", 4));
        tset.insert("one");
        tset.insert("two");
        tset.insert("three");
        tset.insert("four");

        KSS_ASSERT(contains(tmap, "two"));
        KSS_ASSERT(!contains(tmap, "five"));
        KSS_ASSERT(contains(tset, "two"));
        KSS_ASSERT(!contains(tset, "five"));
    }),
    make_pair("hasAtLeast", [] {
        vector<string> v { "one", "two", "three", "four" };
        KSS_ASSERT(hasAtLeast(v, 3));
        KSS_ASSERT(hasAtLeast(v, 4));
        KSS_ASSERT(!hasAtLeast(v, 5));
    }),
    make_pair("isFull", [] {
        vector<int> v;
        v.reserve(10);
        KSS_ASSERT(!isFull(v));
        for (size_t i = 0; i < (v.capacity()-1); ++i) { v.push_back((int)i); }
        KSS_ASSERT(!isFull(v));
        v.push_back(-1);
        KSS_ASSERT(isFull(v));
    }),
    make_pair("eraseIf", [] {
        vector<int> v { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        eraseIf(v, [](int i){ return (i % 3) == 0; });
        KSS_ASSERT(v.size() == 7);
        vector<int> vCheck { 1, 2, 4, 5, 7, 8, 10 };
        KSS_ASSERT(equal(v.begin(), v.end(), vCheck.begin()));

        v = vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        eraseIf(v, [](int i){ return (i >= 7); });
        KSS_ASSERT(v.size() == 6);
        vCheck = vector<int>{ 1, 2, 3, 4, 5, 6 };
        KSS_ASSERT(equal(v.begin(), v.end(), vCheck.begin()));

        v = vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        eraseIf(v, [](int){ return false; });
        KSS_ASSERT(v.size() == 10);
        vCheck = vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        KSS_ASSERT(equal(v.begin(), v.end(), vCheck.begin()));

        v = vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        eraseIf(v, [](int){ return true; });
        KSS_ASSERT(v.size() == 0);
    }),
    make_pair("findIf/containsIf", [] {
        vector<int> v { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        auto it = findIf(v, [](int i) { return i == 5; });
        KSS_ASSERT(it != v.end());
        KSS_ASSERT(*it == 5);
        *it = -5;
        KSS_ASSERT(v[4] == -5);
        KSS_ASSERT(findInIterator(findIf(v, [](int i) { return i == 6; })) == 6);
        KSS_ASSERT(findIf(v, [](int i) { return i == 5; }) == v.end());
        KSS_ASSERT(containsIf(v, [](int i) { return i == 6; }) == true);
        KSS_ASSERT(containsIf(v, [](int i) { return i == -1; }) == false);
    }),
    make_pair("apply", [] {
        KSS_ASSERT(applyToVectorLikeContainer<vector<int>>());
        KSS_ASSERT(applyToVectorLikeContainer<deque<int>>());
        KSS_ASSERT(applyToVectorLikeContainer<valarray<int>>());
    })
});
