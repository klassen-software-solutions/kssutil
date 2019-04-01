//
//  uuid.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-08-28.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//

#include <kss/util/stringutil.hpp>
#include <kss/util/uuid.hpp>

#include "ksstest.hpp"


using namespace std;
using namespace kss::util;
using namespace kss::test;


static void basic_tests() {
    const char* suid3 = "1b4e28ba-2fa1-11d2-883f-b9a761bde3fb";
    const char* suid4 = "1b4e28ba-3fa1-11d2-883f-b9a761bde3fb";
    uuid_t uid3, uid4, uid;
    uuid_parse(suid3, uid3);
    uuid_parse(suid4, uid4);

    UUID u1;
    KSS_ASSERT(!u1);
    KSS_ASSERT(u1 == UUID::null());
    KSS_ASSERT(string(u1) == "");
    
    UUID u2(u1);
    KSS_ASSERT(!bool(u2));
    KSS_ASSERT(u2 == UUID::null());
    KSS_ASSERT(u1 == u2);
    KSS_ASSERT(u1 <= u2);
    KSS_ASSERT(u2 >= u1);
    
    UUID u3(uid3);
    KSS_ASSERT((bool)u3);
    KSS_ASSERT(u3 != UUID::null());
    KSS_ASSERT(u3 != u1);
    KSS_ASSERT((string)u3 == "1b4e28ba-2fa1-11d2-883f-b9a761bde3fb");
    uuid_copy(uid, u3.value());
    KSS_ASSERT(uuid_compare(uid, uid3) == 0);
    KSS_ASSERT(u3 == UUID(uid));
    
    UUID u4((string)suid4);
    KSS_ASSERT((bool)u4);
    KSS_ASSERT(u4 != UUID::null());
    KSS_ASSERT(u4 != u1 && u4 != u3);
    KSS_ASSERT(u3 != u4);
    KSS_ASSERT(u3 < u4);
    KSS_ASSERT(u3 <= u4);
    KSS_ASSERT(u4 > u3);
    KSS_ASSERT(u4 >= u3);
    
    UUID u = UUID::generate();
    KSS_ASSERT(u != u1 && u != u3 && u != u4);
    u.copyInto(&uid);
    KSS_ASSERT(u == UUID(uid));
    
    u.clear();
    KSS_ASSERT(!u && u == u1 && u == UUID::null());

    KSS_ASSERT(throwsException<invalid_argument>([] {
        UUID("this is not valid");
    }));

    KSS_ASSERT(throwsException<invalid_argument>([&] {
        u.copyInto(nullptr);
    }));
}

static TestSuite ts("::uuid", {
    make_pair("basic tests", basic_tests)
});

