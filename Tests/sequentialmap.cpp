//
//  sequentialmap.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-04-04.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include <kss/util/sequentialmap.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::util;
using namespace kss::util::containers;
using namespace kss::test;

namespace {
    static pair<string, int> p1 = make_pair("one", 1);
    static pair<string, int> p2 = make_pair("one", 2);
    static pair<string, int> ar[] = {
        make_pair("this", 1),
        make_pair("is", 2),
        make_pair("a", 3),
        make_pair("test", 4)
    };


    void test_stage_1(SequentialMap<string, int>& m) {

        const SequentialMap<string, int>& mref(m);
        KSS_ASSERT(!m.empty() && m.size() == 4);
        KSS_ASSERT(equal(m.begin(), m.end(), ar));
        KSS_ASSERT(equal(mref.begin(), mref.end(), ar));
        KSS_ASSERT(equal(m.cbegin(), m.cend(), ar));

        reverse_iterator<pair<string, int>* > rit(ar+4);
        KSS_ASSERT(equal(m.rbegin(), m.rend(), rit));
        KSS_ASSERT(equal(mref.rbegin(), mref.rend(), rit));
        KSS_ASSERT(equal(m.crbegin(), m.crend(), rit));
    }
}


static TestSuite ts("containers::sequentialmap", {
    make_pair("basic tests", [] {
        SequentialMap<string, int> smap;
        KSS_ASSERT(smap.empty());
        smap.insert(ar, ar+4);
        test_stage_1(smap);

        SequentialMap<string, int>::value_compare vcomp = smap.value_comp();
        SequentialMap<string, int>::key_compare kcomp = smap.key_comp();
        KSS_ASSERT(vcomp(p1, p2) == 0);
        KSS_ASSERT(kcomp(p1.first, p2.first) == 0);

        SequentialMap<string, int> smap2(ar, ar+4);
        test_stage_1(smap2);

        SequentialMap<string, int> smap3(smap);
        test_stage_1(smap3);

        smap3.clear();
        KSS_ASSERT(smap3.empty() && smap3.size() == 0);
        test_stage_1(smap3 = smap2);
        test_stage_1(smap3);

        KSS_ASSERT(smap["a"] == 3 && smap.size() == 4);
        KSS_ASSERT((smap["x"] = 5) == 5 && smap.size() == 5);
        KSS_ASSERT(smap.insert(smap.begin(), make_pair("aaargh", 6))->second == 6 && smap.size() == 6);

        const SequentialMap<string, int>& mref(smap);
        KSS_ASSERT(smap.at("a") == 3 && mref.at("a") == 3);
        KSS_ASSERT(throwsException<out_of_range>([&] { smap.at("notthere"); }));
        KSS_ASSERT(throwsException<out_of_range>([&] { mref.at("notthere"); }));
        KSS_ASSERT(smap.size() == 6);

        pair<string, int> ar2[] = {
            make_pair("this", 1),
            make_pair("is", 2),
            make_pair("a", 3),
            make_pair("test", 4),
            make_pair("aaargh", 6)
        };
        smap.erase(smap.find("x"));
        KSS_ASSERT(smap.size() == 5 && equal(smap.begin(), smap.end(), ar2));

        KSS_ASSERT(smap.erase("notthere") == 0 && smap.size() == 5 && equal(smap.begin(), smap.end(), ar2));
        pair<string, int> ar3[] = {
            make_pair("is", 2),
            make_pair("a", 3),
            make_pair("test", 4),
            make_pair("aaargh", 6)
        };
        KSS_ASSERT(smap.erase("this") == 1 && smap.size() == 4 && equal(smap.begin(), smap.end(), ar3));

        smap.erase(smap.find("a"), smap.find("aaargh"));
        pair<string, int> ar4[] = { make_pair("is", 2), make_pair("aaargh", 6) };
        KSS_ASSERT(smap.size() == 2 && equal(smap.begin(), smap.end(), ar4));

        smap.swap(smap2);
        test_stage_1(smap);
        KSS_ASSERT(smap2.size() == 2 && equal(smap2.begin(), smap2.end(), ar4));

        swap(smap2, smap3);
        test_stage_1(smap2);
        KSS_ASSERT(smap3.size() == 2 && equal(smap3.begin(), smap3.end(), ar4));
        
        KSS_ASSERT(smap2.count("notthere") == 0);
        KSS_ASSERT(smap2.count("a") == 1);
        test_stage_1(smap2);
    })
});
