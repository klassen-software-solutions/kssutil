//
//  argumentvector.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-03-31.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

#include <kss/test/all.h>
#include <kss/util/argumentvector.hpp>

using namespace std;
using namespace kss::util::po;
using namespace kss::test;

namespace {
    // Returns true if the ArgumentVector matches the list of arguments.
    bool matches(const ArgumentVector& av, initializer_list<string> args) {
        const auto sz = static_cast<int>(args.size());
        if (av.argc() != sz) {
            return false;
        }
        if (sz == 0 && av.argv() != nullptr) {
            return false;
        }

        int i = 0;
        for (const auto& arg : args) {
            if (av.argv()[i] == nullptr) {
                return false;
            }
            if (strcmp(av.argv()[i], arg.c_str()) != 0) {
                return false;
            }
            ++i;
        }
        return true;
    }
}

static TestSuite ts("po::argumentvector", {
    make_pair("construction", [] {
        KSS_ASSERT(isTrue([] {
            ArgumentVector av;
            return matches(av, {});
        }));
        KSS_ASSERT(isTrue([] {
            ArgumentVector av({ "one", "two", "three", "four" });
            return matches(av, { "one", "two", "three", "four" });
        }));
        KSS_ASSERT(isTrue([] {
            ArgumentVector av1({ "one", "two", "three", "four" });
            ArgumentVector av(move(av1));
            return matches(av, { "one", "two", "three", "four" });
        }));
        KSS_ASSERT(isTrue([] {
            ArgumentVector av1({ "one", "two", "three", "four" });
            ArgumentVector av = move(av1);
            return matches(av, { "one", "two", "three", "four" });
        }));
    }),
    make_pair("add", [] {
        ArgumentVector av;

        av.add("one");
        KSS_ASSERT(matches(av, { "one" }));

        string s("two");
        av.add(move(s));
        KSS_ASSERT(matches(av, { "one", "two" }));

        av.add({ "three", "four", "five" });
        KSS_ASSERT(matches(av, { "one", "two", "three", "four", "five" }));

        vector<string> v { "six", "seven" };
        av.add(v.begin(), v.end());
        KSS_ASSERT(matches(av, { "one", "two", "three", "four", "five", "six", "seven" }));
    })
});
