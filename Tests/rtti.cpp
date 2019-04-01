//
//  rtti.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-01-23.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//
// 	Permission is hereby granted to use, modify, and publish this file without restriction other
// 	than to recognize that others are allowed to do the same.
//

#include <iostream>
#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
// The g++ compiler gives a warning when using isInstanceOf when type C and T are the
// same type. This isn't a problem - the code still gives the correct answer - and it
// is a case that would never really come up in the real world, but is included in the
// unit tests just for completion. So we ignore the error. Note that when we move to
// C++17, we may be able to resolve this through the use of an "if constexpr" statement.
#include <kss/util/rtti.hpp>
#pragma GCC diagnostic pop

#include "ksstest.hpp"


using namespace std;
using namespace kss::util::rtti;
using namespace kss::test;


namespace {
    class A {
    public:
        A() {}
        virtual ~A() {}
    };

    class B : public A {
    public:
        B() {}
        virtual ~B() {}
    };

    class C {
    public:
        C() {}
    };
}

class ClassInNoNamespace {
public:
    ClassInNoNamespace() {}
};

namespace myspace {
    class AnotherClass {
    public:
        AnotherClass() {}
    };
}


static TestSuite ts("rtti::rtti", {
    make_pair("name", [] {
        A a;
        int i = 0;
        int& iref = i;

        KSS_ASSERT(name<string>() == "std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >"    // clang/Xcode
                   || name<string>() == "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"            // gcc
                   || name<string>() == "std::string");                                                                                // what it really should be
        KSS_ASSERT(name<myspace::AnotherClass>() == "myspace::AnotherClass");
        KSS_ASSERT(name<A>() == "(anonymous namespace)::A");
        KSS_ASSERT(name(a) == "(anonymous namespace)::A");
        KSS_ASSERT(name<ClassInNoNamespace>() == "ClassInNoNamespace");
        KSS_ASSERT(name<int>() == "int");
        KSS_ASSERT(name(i) == "int");
        KSS_ASSERT(name(&i) == "int*");
        KSS_ASSERT(name(iref) == "int");
        KSS_ASSERT(name<unsigned>() == "unsigned int");
        KSS_ASSERT(name(10L) == "long");
        KSS_ASSERT(name<unsigned long>() == "unsigned long");
    }),
    make_pair("isInstanceOf", [] {
        A a;
        B b;
        KSS_ASSERT(isInstanceOf<B>(b));
        KSS_ASSERT(isInstanceOf<A>(b));
        KSS_ASSERT(!isInstanceOf<B>(a));
        KSS_ASSERT(isInstanceOfPtr<B>(&b));
        KSS_ASSERT(isInstanceOfPtr<A>(&b));
        KSS_ASSERT(!isInstanceOfPtr<B>(&a));
        KSS_ASSERT(!isInstanceOfPtr<B>((B*)nullptr));
    }),
    make_pair("as", [] {
        KSS_ASSERT(as<B>((A*)nullptr) == nullptr);
        B b;
        {
            A* aptr = &b;
            KSS_ASSERT(as<B>(aptr) != nullptr);
            KSS_ASSERT(typeid(as<B>(aptr)) == typeid(B*));
            KSS_ASSERT(as<C>(aptr) == nullptr);
        }
        {
            const A* aptr = &b;
            KSS_ASSERT(as<B>(aptr) != nullptr);
            KSS_ASSERT(typeid(as<B>(aptr)) == typeid(const B*));
            KSS_ASSERT(as<C>(aptr) == nullptr);
        }
        {
            A& aref = b;
            KSS_ASSERT(as<B>(aref) != nullptr);
            KSS_ASSERT(typeid(as<B>(aref)) == typeid(B*));
            KSS_ASSERT(as<C>(aref) == nullptr);
        }
        {
            const A& aref = b;
            KSS_ASSERT(as<B>(aref) != nullptr);
            KSS_ASSERT(typeid(as<B>(aref)) == typeid(const B*));
            KSS_ASSERT(as<C>(aref) == nullptr);
        }
    }),
    make_pair("demangle", [] {
        KSS_ASSERT(throwsException<runtime_error>( [] {
            demangle("thisshouldnotbeavalidname#*$*$*%");
        }));
    })
});


