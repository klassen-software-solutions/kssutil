//
//  no_parallel.hpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-01-18.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#ifndef no_parallel_h
#define no_parallel_h

#include <string>

#include <kss/test/all.h>

using namespace std;
using namespace kss::test;


// We are suppressing the standard error device in a couple of places. For that to
// work reliably we cannot run this test in parallel with others that also attempt
// to suppress the stream.
class NoParallelTestSuite : public TestSuite, public MustNotBeParallel {
public:
    NoParallelTestSuite(const string& name, test_case_list_t fns)
    : TestSuite(name, fns)
    {}
};


#endif /* no_parallel_h */
