//
//  attributes.cpp
//  kssutiltest
//
//  Created by Steven W. Klassen on 2018-06-18.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <iostream>
#include <stdexcept>
#include <system_error>

#include <kss/util/attributes.hpp>
#include <kss/util/timeutil.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::util;
using namespace kss::test;

namespace {
    class MyClass : public Attributes {
    public:
        void addKey3() {
            attributes()["key3"] = "333";
        }
    };
}

static TestSuite ts("::attributes", {
    make_pair("basic tests", [] {
        // Attributes that exist.
        MyClass mc;
        mc.setAttribute("key1", "-111");
        KSS_ASSERT(mc.hasAttribute("key1"));
        KSS_ASSERT(mc.attribute<string>("key1") == "-111");
        KSS_ASSERT(mc.attribute<int>("key1") == -111);
        KSS_ASSERT(mc.attributeWithDefault("key1", 222) == -111);

        KSS_ASSERT(throwsException<system_error>([&] {
            mc.attribute<unsigned>("key1");
        }));
        KSS_ASSERT(throwsException<system_error>([&] {
            mc.attributeWithDefault("key1", 222U);
        }));

        const MyClass& constRef = mc;
        const auto& attributeMap = constRef.attributes();
        KSS_ASSERT(attributeMap.size() == 1);

        const auto keys = mc.attributeKeys();
        KSS_ASSERT(keys.size() == 1);
        KSS_ASSERT(keys[0] == "key1");

        // Attributes that do not exist.
        KSS_ASSERT(!mc.hasAttribute("key2"));
        KSS_ASSERT(mc.attributeWithDefault("key2", string("222")) == "222");
        KSS_ASSERT(mc.attributeWithDefault("key2", 222) == 222);
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            mc.attribute<string>("key2");
        }));

        // Contract failsures
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            mc.setAttribute("", "some value");
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            mc.attribute<string>("");
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            mc.attributeWithDefault<string>("", "hi");
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            mc.hasAttribute("");
        }));

        // Protected items.
        mc.addKey3();
        KSS_ASSERT(mc.attributeKeys().size() == 2);
        KSS_ASSERT(mc.attribute<unsigned>("key3") == 333U);
    }),
    make_pair("time types", [] {
        // time_point
        using timestamp_t = chrono::time_point<chrono::system_clock, chrono::milliseconds>;
        MyClass mc;
        mc.setAttribute("bad", "hi there");
        mc.setAttribute("time", "2010-06-23T12:05:02.502-06:00");

        KSS_ASSERT(mc.attribute<timestamp_t>("time")
                   == time::fromIso8601String<timestamp_t>("2010-06-23T12:05:02.502-06:00"));
        KSS_ASSERT(throwsException<system_error>([&] {
            mc.attribute<timestamp_t>("bad");
        }));

        // duration
        KSS_ASSERT(isEqualTo<int>(10*60*60, [&] {
            mc.setAttribute("duration", "10h");
            return mc.attribute<chrono::seconds>("duration").count();
        }));
    })
});
