//
//  add_rel_ops.cpp
//  kssutiltest
//
//  Created by Steven W. Klassen on 2018-05-01.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//
//     Permission is hereby granted to use, modify, and publish this file without restriction other
//     than to recognize that others are allowed to do the same.
//

#include <kss/util/add_rel_ops.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::util;
using namespace kss::test;

namespace {
    struct MyClass : public AddRelOps<MyClass> {
        explicit MyClass(int v) : val(v) {}
        bool operator==(const MyClass& c) const noexcept { return val == c.val; }
        bool operator<(const MyClass& c) const noexcept { return val < c.val; }

        int val;
    };
}

static TestSuite ts("::add_rel_ops", {
    make_pair("basic tests", [] {
        MyClass t1(1);
        MyClass t2(2);
        KSS_ASSERT(t1 == t1);
        KSS_ASSERT(t1 != t2);
        KSS_ASSERT(t1 < t2);
        KSS_ASSERT(t1 <= t2);
        KSS_ASSERT(t2 > t1);
        KSS_ASSERT(t2 >= t1);
    })
});
