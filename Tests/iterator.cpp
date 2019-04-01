//
//  iterator.cpp
//  kssutil
//
// 	Permission is hereby granted to use, modify, and publish this file without restriction other
// 	than to recognize that others are allowed to do the same.
//
//  Created by Steven W. Klassen on 2014-10-24.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <vector>

#include <kss/util/iterator.hpp>

#include "ksstest.hpp"
#include "no_parallel.hpp"
#include "suppress.hpp"

using namespace std;
using namespace kss::util::iterators;
using namespace kss::test;


namespace {
    class Container {
    public:
        Container() { _current = 0; }
        void increment() { ++_current; }

    protected:
        long _current;
    };

    // subclass for testing the ForwardIterator
    class ForwardContainer : public Container {
    public:
        typedef ForwardIterator<ForwardContainer, unsigned> const_iterator;
        const_iterator begin() { return const_iterator(*this); }
        const_iterator end() { return const_iterator(); }

        void next(unsigned& value) {
            if (_current > 5) { throw runtime_error("too many next() calls"); }
            ++_current;
            value = unsigned(_current);
        }

        bool hasAnother() const {
            if (_current > 5) { throw runtime_error("too many next()s"); }
            if (_current == 5) { return false; }
            return true;
        }
    };

    // class for testing the CopyRandomAccessIterator
    class random_container {
    public:
        typedef int value_type;
        typedef ptrdiff_t difference_type;
        typedef CopyRandomAccessIterator<random_container> const_iterator;
        const_iterator begin() { return const_iterator(*this, false); }
        const_iterator end() { return const_iterator(*this, true); }

        size_t size() const { return 5; }
        int operator[](size_t i) const { return int(i); }
    };
}

static void testForwardIterator() {
    {   // Basic tests.
        ForwardContainer c;
        ForwardContainer::const_iterator it = c.begin();
        ForwardContainer::const_iterator last = c.end();
        for (unsigned i = 1; i <= 5; ++i) {
            KSS_ASSERT(it != last);
            KSS_ASSERT(*it == i);
            ++it;
        }
        KSS_ASSERT(it == last);
        KSS_ASSERT(c.begin() == last);
    }
    {   // Test type declarations.
        ForwardContainer c;
        auto it = c.begin();
        ForwardContainer::const_iterator::difference_type d = 1;
        ForwardContainer::const_iterator::value_type v = *it;
        ForwardContainer::const_iterator::pointer p = &v;
        ForwardContainer::const_iterator::reference r = v;
        KSS_ASSERT(typeid(d) == typeid(ptrdiff_t));
        KSS_ASSERT(typeid(v) == typeid(unsigned));
        KSS_ASSERT(typeid(p) == typeid(const unsigned*));
        KSS_ASSERT(typeid(r) == typeid(const unsigned&));
    }
    {   // Test a variety of copies.
        ForwardContainer c1, c2;
        ForwardContainer::const_iterator it1 = c1.begin(), it2 = c2.begin();
        ForwardContainer::const_iterator last1 = c1.end(), last2 = c2.end();
        KSS_ASSERT(it1 != it2 && !(it1 == it2));
        KSS_ASSERT(it1 != last1 && !(it1 == last1));
        KSS_ASSERT(last1 == last2 && !(last1 != last2));

        ForwardContainer::const_iterator it = it2;
        KSS_ASSERT(it == it2);
        KSS_ASSERT(it != it1);

        KSS_ASSERT(*it == 1 && *it2 == 1);  // Note that two input iterators from the same container
        ++it2;                              // will "interfere" with each other in terms of reading
        ++it;                               // values from the container.
        KSS_ASSERT(*it == 3 && *it2 == 2);
        ++it;
        ++it;
        KSS_ASSERT(*it == 5 && *it2 == 2);
        ++it2;
        KSS_ASSERT(it2 == last2);

        // Test that we terminate when we iterate past end().
        suppress(cerr, [&] {
            KSS_ASSERT(terminates([&] { ++last1; }));
            KSS_ASSERT(terminates([&] { ++it2; }));
            ++it;
            KSS_ASSERT(it == last2);
            KSS_ASSERT(terminates([&] { ++it; }));
        });

        // Test that we pass on exceptions from operator>>() and eof().
        for (int i = 0; i < 10; ++i) { c1.increment(); }
        KSS_ASSERT(throwsException<runtime_error>([&] { ++it1; }));
    }
    {   // Test prefix iteration combined with a dereference.
        ForwardContainer c;
        auto it = c.begin();
        for (unsigned i = 1; i < 5;) {
            KSS_ASSERT(*++it == ++i);
        }
    }
    {   // Test postfix iteration combined with a dereference.
        ForwardContainer c;
        unsigned i = 1;
        for (auto it = c.begin(), last = c.end(); it != last;) {
            KSS_ASSERT(*it++ == i++);
        }
    }
    {   // Test that swap will handle our end() iterators.
        ForwardContainer c, c1;
        auto first = c.begin(), last = c.end(), first1 = c1.begin();
        swap(first, last);
        KSS_ASSERT(first == c.end());
        swap(first, first1);
        KSS_ASSERT(first1 == c.end());
    }
    {   // Test that it will work with STL algorithms.
        unsigned ar[10];
        ForwardContainer c;
        for (size_t i = 0; i < 10; ++i) ar[i] = 999;
        copy(c.begin(), c.end(), ar);
        for (size_t i = 0; i < 5; ++i) {
            KSS_ASSERT(ar[i] == (unsigned)i+1);
        }
        for (size_t i = 5; i < 10; ++i) {
            KSS_ASSERT(ar[i] == 999);
        }
    }
}

static void testRandomAccessIterator() {
    {   // Basic tests.
        vector<int> v = { 1, 2, 3, 4, 5 };
        RandomAccessIterator<vector<int>> first(v, false);
        RandomAccessIterator<vector<int>> last(v, true);
        RandomAccessIterator<vector<int>> it = first;
        for (int i = 1 ; i <= 5; ++i) {
            KSS_ASSERT(it != last);
            KSS_ASSERT(*it == i);
            ++it;
        }
        KSS_ASSERT(it == last);
        KSS_ASSERT(first != last);
        int i = 1;
        for (it = first; it != last; ++it) {
            KSS_ASSERT(*it == i++);
        }
    }
    {   // Test type declarations.
        vector<int> v = { 1, 2, 3, 4, 5 };
        RandomAccessIterator<vector<int>> first(v, false);
        RandomAccessIterator<vector<int>> last(v, true);
        RandomAccessIterator<vector<int>>::difference_type d = last - first;
        RandomAccessIterator<vector<int>>::value_type vt = *first;
        RandomAccessIterator<vector<int>>::pointer p = &vt;
        RandomAccessIterator<vector<int>>::reference r = *first;
        KSS_ASSERT(d == 5);
        KSS_ASSERT(vt == 1);
        KSS_ASSERT(*p == 1);
        KSS_ASSERT(r == 1);
    }
    {   // Test constructors, copying, and swapping.
        vector<int> v = { 1, 2, 3, 4, 5 };
        RandomAccessIterator<vector<int>> first(v, false);
        RandomAccessIterator<vector<int>> last(v, true);
        RandomAccessIterator<vector<int>> it(first);
        KSS_ASSERT(it == first && it != last);
        it = last;
        KSS_ASSERT(it != first && it == last);

        it.swap(first);
        KSS_ASSERT(first == last && it != last);
        swap(it, first);
        KSS_ASSERT(first != last && it == last);
    }
    {   // Test inequality operators.
        vector<int> v = { 1, 2, 3, 4, 5 };
        RandomAccessIterator<vector<int>> first(v, false);
        RandomAccessIterator<vector<int>> last(v, true);
        RandomAccessIterator<vector<int>> it = first;
        KSS_ASSERT(it == first && it <= first && it >= first);
        KSS_ASSERT(it != last && it < last && last > it);
    }
    {   // Test dereferencing.
        vector<int> v = { 1, 2, 3, 4, 5 };
        RandomAccessIterator<vector<int>> first(v, false);
        KSS_ASSERT(*first == 1);
        const int& i = *first;
        KSS_ASSERT(i == 1);
        KSS_ASSERT(first[2] == 3);
        first[2] = -3;
        KSS_ASSERT(v[2] == -3);
    }
    {   // Test pointer arithmetic.
        vector<int> v = { 1, 2, 3, 4, 5 };
        RandomAccessIterator<vector<int>> first(v, false);
        RandomAccessIterator<vector<int>> last(v, true);
        RandomAccessIterator<vector<int>> it = first;
        it++; it++; it++;
        KSS_ASSERT((it - first) == 3);
        --it;
        KSS_ASSERT((it - first) == 2);
        it += 3;
        KSS_ASSERT(it == last);
        it -= 1;
        KSS_ASSERT(*it == 5);
        KSS_ASSERT(*(it - 2) == 3);
        KSS_ASSERT(*(1 + first) == 2);
        KSS_ASSERT(*(first + 3) == 4);
        it += 1;
        KSS_ASSERT(it == last);
        it -= 5;
        KSS_ASSERT(it == first);
        ++it;
        KSS_ASSERT(*it == 2);
    }
    {   // Test pointer dereferencing.
        vector<pair<string, int>> v;
        v.push_back(make_pair("one", 1));
        v.push_back(make_pair("two", 2));
        v.push_back(make_pair("three", 3));
        RandomAccessIterator<vector<pair<string, int>>> it(v, false);
        ++it;
        KSS_ASSERT((*it).first == "two");
        KSS_ASSERT(it->second == 2);
        it->second = -2;
        KSS_ASSERT(v[1].second == -2);
        KSS_ASSERT(it[1].first == "three");
        it[1].first = "THREE";
        KSS_ASSERT(v[2].first == "THREE");
    }
}

static void testConstRandomAccessIterator() {
    {   // Basic tests.
        vector<int> v = { 1, 2, 3, 4, 5 };
        ConstRandomAccessIterator<vector<int>> first(v, false);
        ConstRandomAccessIterator<vector<int>> last(v, true);
        ConstRandomAccessIterator<vector<int>> it = first;
        for (int i = 1 ; i <= 5; ++i) {
            KSS_ASSERT(it != last);
            KSS_ASSERT(*it == i);
            ++it;
        }
        KSS_ASSERT(it == last);
        KSS_ASSERT(first != last);
        int i = 1;
        for (it = first; it != last; ++it) {
            KSS_ASSERT(*it == i++);
        }
    }
    {   // Test type declarations.
        vector<int> v = { 1, 2, 3, 4, 5 };
        ConstRandomAccessIterator<vector<int>> first(v, false);
        ConstRandomAccessIterator<vector<int>> last(v, true);
        ConstRandomAccessIterator<vector<int>>::difference_type d = last - first;
        ConstRandomAccessIterator<vector<int>>::value_type vt = *first;
        ConstRandomAccessIterator<vector<int>>::pointer p = &vt;
        ConstRandomAccessIterator<vector<int>>::reference r = *first;
        KSS_ASSERT(d == 5);
        KSS_ASSERT(vt == 1);
        KSS_ASSERT(*p == 1);
        KSS_ASSERT(r == 1);
    }
    {   // Test constructors, copying, and swapping.
        vector<int> v = { 1, 2, 3, 4, 5 };
        ConstRandomAccessIterator<vector<int>> first(v, false);
        ConstRandomAccessIterator<vector<int>> last(v, true);
        ConstRandomAccessIterator<vector<int>> it(first);
        KSS_ASSERT(it == first && it != last);
        it = last;
        KSS_ASSERT(it != first && it == last);

        it.swap(first);
        KSS_ASSERT(first == last && it != last);
        swap(it, first);
        KSS_ASSERT(first != last && it == last);
    }
    {   // Test inequality operators.
        vector<int> v = { 1, 2, 3, 4, 5 };
        ConstRandomAccessIterator<vector<int>> first(v, false);
        ConstRandomAccessIterator<vector<int>> last(v, true);
        ConstRandomAccessIterator<vector<int>> it = first;
        KSS_ASSERT(it == first && it <= first && it >= first);
        KSS_ASSERT(it != last && it < last && last > it);
    }
    {   // Test dereferencing.
        vector<int> v = { 1, 2, 3, 4, 5 };
        ConstRandomAccessIterator<vector<int>> first(v, false);
        KSS_ASSERT(*first == 1);
        const int& i = *first;
        KSS_ASSERT(i == 1);
        KSS_ASSERT(first[2] == 3);
    }
    {   // Test pointer arithmetic.
        vector<int> v = { 1, 2, 3, 4, 5 };
        ConstRandomAccessIterator<vector<int>> first(v, false);
        ConstRandomAccessIterator<vector<int>> last(v, true);
        ConstRandomAccessIterator<vector<int>> it = first;
        it++; it++; it++;
        KSS_ASSERT((it - first) == 3);
        --it;
        KSS_ASSERT((it - first) == 2);
        it += 3;
        KSS_ASSERT(it == last);
        it -= 1;
        KSS_ASSERT(*it == 5);
        KSS_ASSERT(*(it - 2) == 3);
        KSS_ASSERT(*(1 + first) == 2);
        KSS_ASSERT(*(first + 3) == 4);
        it += 1;
        KSS_ASSERT(it == last);
        it -= 5;
        KSS_ASSERT(it == first);
        ++it;
        KSS_ASSERT(*it == 2);
    }
    {   // Test pointer dereferencing.
        vector<pair<string, int>> v;
        v.push_back(make_pair("one", 1));
        v.push_back(make_pair("two", 2));
        v.push_back(make_pair("three", 3));
        ConstRandomAccessIterator<vector<pair<string, int>>> it(v, false);
        ++it;
        KSS_ASSERT((*it).first == "two");
        KSS_ASSERT(it->second == 2);
        KSS_ASSERT(it[1].first == "three");
    }
}


static void testCopyRandomAccessIterator() {
    {   // Basic tests.
        vector<int> v = { 1, 2, 3, 4, 5 };
        CopyRandomAccessIterator<vector<int>> first(v, false);
        CopyRandomAccessIterator<vector<int>> last(v, true);
        CopyRandomAccessIterator<vector<int>> it = first;
        for (int i = 1 ; i <= 5; ++i) {
            KSS_ASSERT(it != last);
            KSS_ASSERT(*it == i);
            ++it;
        }
        KSS_ASSERT(it == last);
        KSS_ASSERT(first != last);
        int i = 1;
        for (it = first; it != last; ++it) {
            KSS_ASSERT(*it == i++);
        }
    }
    {   // Test type declarations.
        vector<int> v = { 1, 2, 3, 4, 5 };
        CopyRandomAccessIterator<vector<int>> first(v, false);
        CopyRandomAccessIterator<vector<int>> last(v, true);
        CopyRandomAccessIterator<vector<int>>::difference_type d = last - first;
        CopyRandomAccessIterator<vector<int>>::value_type vt = *first;
        CopyRandomAccessIterator<vector<int>>::pointer p;
        KSS_ASSERT(d == 5);
        KSS_ASSERT(vt == 1);
    }
    {   // Test constructors, copying, and swapping.
        vector<int> v = { 1, 2, 3, 4, 5 };
        CopyRandomAccessIterator<vector<int>> first(v, false);
        CopyRandomAccessIterator<vector<int>> last(v, true);
        CopyRandomAccessIterator<vector<int>> it(first);
        KSS_ASSERT(it == first && it != last);
        it = last;
        KSS_ASSERT(it != first && it == last);

        it.swap(first);
        KSS_ASSERT(first == last && it != last);
        swap(it, first);
        KSS_ASSERT(first != last && it == last);
    }
    {   // Test inequality operators.
        vector<int> v = { 1, 2, 3, 4, 5 };
        CopyRandomAccessIterator<vector<int>> first(v, false);
        CopyRandomAccessIterator<vector<int>> last(v, true);
        CopyRandomAccessIterator<vector<int>> it = first;
        KSS_ASSERT(it == first && it <= first && it >= first);
        KSS_ASSERT(it != last && it < last && last > it);
    }
    {   // Test dereferencing.
        vector<int> v = { 1, 2, 3, 4, 5 };
        CopyRandomAccessIterator<vector<int>> first(v, false);
        KSS_ASSERT(*first == 1);
        const int& i = *first;
        KSS_ASSERT(i == 1);
        KSS_ASSERT(first[2] == 3);
    }
    {   // Test pointer arithmetic.
        vector<int> v = { 1, 2, 3, 4, 5 };
        CopyRandomAccessIterator<vector<int>> first(v, false);
        CopyRandomAccessIterator<vector<int>> last(v, true);
        CopyRandomAccessIterator<vector<int>> it = first;
        it++; it++; it++;
        KSS_ASSERT((it - first) == 3);
        --it;
        KSS_ASSERT((it - first) == 2);
        it += 3;
        KSS_ASSERT(it == last);
        it -= 1;
        KSS_ASSERT(*it == 5);
        KSS_ASSERT(*(it - 2) == 3);
        KSS_ASSERT(*(1 + first) == 2);
        KSS_ASSERT(*(first + 3) == 4);
        it += 1;
        KSS_ASSERT(it == last);
        it -= 5;
        KSS_ASSERT(it == first);
        ++it;
        KSS_ASSERT(*it == 2);
    }
    {   // Test pointer dereferencing.
        vector<pair<string, int>> v;
        v.push_back(make_pair("one", 1));
        v.push_back(make_pair("two", 2));
        v.push_back(make_pair("three", 3));
        CopyRandomAccessIterator<vector<pair<string, int>>> it(v, false);
        ++it;
        KSS_ASSERT((*it).first == "two");
        KSS_ASSERT(it->second == 2);
        KSS_ASSERT(it[1].first == "three");
    }
    {   // Test a copy iterator based on a container that generates instead of contains data.
        random_container c;
        int i = 0;
        for (auto it = c.begin(), last = c.end(); it != last; ++it, ++i) {
            KSS_ASSERT(*it == i);
        }
    }
}

static NoParallelTestSuite ts("iterators::iterator", {
    make_pair("ForwardIterator", testForwardIterator),
    make_pair("RandomAccessIterator", testRandomAccessIterator),
    make_pair("ConstRandomAccessIterator", testConstRandomAccessIterator),
    make_pair("CopyRandomAccessIterator", testCopyRandomAccessIterator)
});
