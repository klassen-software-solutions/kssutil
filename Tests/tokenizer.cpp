//
//  tokenizer.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-01-18.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <iostream>

#include <kss/test/all.h>
#include <kss/util/tokenizer.hpp>

#include "no_parallel.hpp"
#include "suppress.hpp"

using namespace std;
using namespace kss::util::strings;
using namespace kss::test;


static NoParallelTestSuite ts("strings::tokenizer", {
    make_pair("simple test", [] {
        Tokenizer t("the quick brown fox");
        string s;
        KSS_ASSERT(isEqualTo<string>("the", [&] { return t.next(s); }));
        KSS_ASSERT(isEqualTo<string>("quick", [&] { return t.next(s); }));
        KSS_ASSERT(isEqualTo<string>("brown", [&] { return t.next(s); }));
        KSS_ASSERT(t.hasAnother());
        KSS_ASSERT(isEqualTo<string>("fox", [&] { return t.next(s); }));
        KSS_ASSERT(!t.hasAnother());
        suppress(cerr, [&] {
            KSS_ASSERT(terminates([&] { t.next(s); }));
        });

        KSS_ASSERT(throwsException<invalid_argument>([] {
            Tokenizer t2("hi", "");
        }));
    }),
    make_pair("empty tokens", [] {
        Tokenizer t("the  quick\nbrown\t\tfox", " \t\n");
        string tokens[] = { "the", "", "quick", "brown", "", "fox" };
        size_t i = 0;
        for (string s : t) {
            KSS_ASSERT(s == tokens[i++]);
        }
    }),
    make_pair("non-zero start", [] {
        Tokenizer t("skip the first bit skip", " ", 5, 19);
        Tokenizer::iterator it = t.begin();
        KSS_ASSERT(*it++ == "the");
        KSS_ASSERT(*it++ == "first");
        KSS_ASSERT(*it++ == "bit");
        KSS_ASSERT(it != t.end());
        KSS_ASSERT(*it++ == "");
        KSS_ASSERT(it == t.end());
    }),
    make_pair("whitespace", [] {
        Tokenizer t(" ", " ");
        auto it = t.begin();
        KSS_ASSERT(*it++ == "");
        KSS_ASSERT(it != t.end());
        KSS_ASSERT(*it++ == "");
        KSS_ASSERT(it == t.end());
    }),
    make_pair("empty", [] {
        Tokenizer t("", " ");
        KSS_ASSERT(t.begin() == t.end());
    })
});
