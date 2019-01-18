//
//  version.cpp
//  unittest
//
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/util/version.hpp>
#include "ksstest.hpp"

using namespace std;
using namespace kss::test;


static TestSuite ts("version", {
    make_pair("version", [] {
        KSS_ASSERT(!kss::util::version().empty());
        KSS_ASSERT(!kss::util::license().empty());
    })
});
