//
//  memory.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-11-23.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <memory>

#include <kss/test/all.h>
#include <kss/util/memory.hpp>

using namespace std;
using namespace kss::util::memory;
using namespace kss::test;

namespace {
    static unsigned int constructed = 0;
    static unsigned int destructed = 0;

    class MyClass {
    public:
        MyClass() { ++constructed; }
        ~MyClass() { ++destructed; }
    };
}

static TestSuite ts("memory::memory", {
    make_pair("null deletor", [] {
        KSS_ASSERT(isTrue([] {
            MyClass mc3;
            {
                shared_ptr<MyClass> mc(new MyClass());
                shared_ptr<MyClass> mc2(&mc3, kss::util::memory::NullDelete<MyClass>());
            }
            return (constructed == 2 && destructed == 1);
        }));
        KSS_ASSERT(constructed == 2 && destructed == 2);
    })
});
