//
//  circular_array.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-12-18.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <iostream>
#include <string>
#include <vector>

#include <kss/util/circular_array.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::util::containers;
using namespace kss::test;

namespace {
    void next_values(CircularArray<int>& ca,
                     int next,
                     int numToAdd,
                     int numToRemove,
                     bool autoGrow = false)
    {
        if (autoGrow) { ca.reserve(ca.size() + (size_t)numToAdd); }
        for (int i = 0; i < numToAdd; ++i) { ca.push_back(next+i); }
        for (int i = 0; i < numToRemove; ++i) { ca.pop_front(); }
    }

    bool check_values(const CircularArray<int>& ca, int first, int size) {
        if (ca.size() != (size_t)size) {
            return false;
        }
        for (int i = 0; i < size; ++i) {
            if (ca[(size_t)i] != (first+i)) {
                return false;
            }
        }
        return true;
    }
}


static TestSuite ts("containers::CircularArray", {
    make_pair("constructors", [] {
        {
            CircularArray<int> ca;
            KSS_ASSERT(ca.size() == 0);
            KSS_ASSERT(ca.empty());
            KSS_ASSERT(ca.capacity() >= 10);
            KSS_ASSERT(ca.get_allocator() == std::allocator<int>());
        }
        {
            CircularArray<int> ca(CircularArray<int>::size_type(5), 2);
            KSS_ASSERT(!ca.empty());
            KSS_ASSERT(ca.size() == 5);
            KSS_ASSERT(ca.capacity() >= 10);
            for (int i : ca) { KSS_ASSERT(i == 2); }

            CircularArray<int> ca2(CircularArray<int>::size_type(15), 2);
            KSS_ASSERT(!ca2.empty());
            KSS_ASSERT(ca2.size() == 15);
            KSS_ASSERT(ca2.capacity() >= 15);
            for (int i : ca2) { KSS_ASSERT(i == 2); }
        }
        {
            int ar[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
            CircularArray<int> ca(ar, ar+5);
            KSS_ASSERT(ca.size() == 5);
            KSS_ASSERT(ca.capacity() >= 10);
            for (int i = 1; i <= 5; ++i) { KSS_ASSERT(ca[size_t(i-1)] == i); }

            CircularArray<int> ca2(ar, ar+20);
            KSS_ASSERT(ca2.size() == 20);
            KSS_ASSERT(ca2.capacity() >= 20);
            for (int i = 1; i <= 20; ++i) { KSS_ASSERT(ca2[size_t(i-1)] == i); }

            CircularArray<int> ca3(ca);
            KSS_ASSERT(ca3 == ca);

            CircularArray<int> ca4(ca, 200);
            KSS_ASSERT(ca4 == ca);
            KSS_ASSERT(ca4.capacity() >= 200);

            CircularArray<int> ca5(std::move(ca4));
            KSS_ASSERT(ca5 == ca);
            KSS_ASSERT(ca5.capacity() >= 200);

            CircularArray<int> ca6 { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
            KSS_ASSERT(ca6 == ca2);
        }
    }),
    make_pair("assignment", [] {
        CircularArray<long> ca { 1L, 2L, 3L, 4L, 5L };
        CircularArray<long> ca2;
        ca2 = ca;
        KSS_ASSERT(ca2 == ca);
        KSS_ASSERT(ca2.capacity() >= ca.capacity());

        CircularArray<long> ca3 = std::move(ca2);
        KSS_ASSERT(ca3 == ca);

        vector<long> v { 10L, 11L, 12L };
        ca3.assign(v.begin(), v.end());
        KSS_ASSERT(ca3.size() == 3 && std::equal(v.begin(), v.end(), ca3.begin()));

        ca3.assign(size_t(25), 200L);
        KSS_ASSERT(ca3.size() == 25);
        for (size_t i = 0; i < 25; ++i) { KSS_ASSERT(ca3[i] == 200L); }

        ca3.assign({ 10L, 11L, 12L });
        KSS_ASSERT(ca3.size() == 3 && std::equal(v.begin(), v.end(), ca3.begin()));
    }),
    make_pair("iterators", [] {
        CircularArray<long> ca { 1L, 2L, 3L, 4L, 5L };
        for (CircularArray<long>::iterator it = ca.begin(), last = ca.end(); it != last; ++it) {
            *it += 100L;
        }
        for (auto i = 101; i <= 105; ++i) { KSS_ASSERT(ca[i-101] == i); }

        long l = 101L;
        const CircularArray<long>& cref = ca;
        for (CircularArray<long>::const_iterator it = cref.begin(); it != cref.end(); ++it) {
            KSS_ASSERT(*it == l++);
        }

        l = 101L;
        for (CircularArray<long>::const_iterator it = ca.cbegin(); it != ca.cend(); ++it) {
            KSS_ASSERT(*it == l++);
        }

        l = 5L;
        for (CircularArray<long>::reverse_iterator it = ca.rbegin(); it != ca.rend(); ++it) {
            *it -= 100L;
            KSS_ASSERT(*it = l--);
        }

        l = 5L;
        for (CircularArray<long>::const_reverse_iterator it = cref.rbegin(); it != cref.rend(); ++it) {
            KSS_ASSERT(*it == l--);
        }

        l = 5L;
        for (CircularArray<long>::const_reverse_iterator it = ca.crbegin(); it !=  ca.crend(); ++it) {
            KSS_ASSERT(*it == l--);
        }
    }),
    make_pair("capacity", [] {
        CircularArray<long> ca { 1L, 2L, 3L, 4L, 5L };
        KSS_ASSERT(ca.size() == 5 && ca.capacity() >= 10);
        KSS_ASSERT(ca.max_size() >= 100);

        ca.resize(3);
        KSS_ASSERT(ca.size() == 3 && ca.capacity() >= 10);

        ca.resize(10, 100L);
        KSS_ASSERT(ca.size() == 10);
        KSS_ASSERT(!ca.empty());
        KSS_ASSERT(ca[0] == 1L && ca[1] == 2L && ca[2] == 3L);
        for (size_t i = 3; i < 10; ++i) { KSS_ASSERT(ca[i] == 100L); }
        KSS_ASSERT(ca.capacity() >= 10);

        ca.reserve(100);
        KSS_ASSERT(ca.size() == 10 && ca.capacity() >= 100);

        ca.shrink_to_fit();
        KSS_ASSERT(ca.size() == 10 && ca.capacity() == 10);
    }),
    make_pair("accessors", [] {
        CircularArray<long> ca { 1L, 2L, 3L, 4L, 5L };
        const CircularArray<long> &cref = ca;
        KSS_ASSERT(ca[2] == 3L);
        KSS_ASSERT(cref[3] == 4L);

        ca[2] = -3L;
        KSS_ASSERT(ca[2] == -3L);

        ca.at(2) = 3L;
        KSS_ASSERT(ca[2] == 3L);
        KSS_ASSERT(cref.at(2) == 3L);
        KSS_ASSERT(throwsException<out_of_range>([&] { ca.at(5); }));
        KSS_ASSERT(throwsException<out_of_range>([&] { cref.at(6); }));

        ca.front() = -1L;
        KSS_ASSERT(ca[0] == -1L && cref.front() == -1L);

        ca.back() = -5L;
        KSS_ASSERT(ca[4] == -5L && cref.back() == -5L);
    }),
    make_pair("modifiers", [] {
        {
            // Simple checks
            CircularArray<string> ca { "one", "two", "three" };
            ca.push_back("four");
            string s("five");
            ca.push_back(std::move(s));
            KSS_ASSERT(ca.size() == 5 && ca[3] == "four" && ca[4] == "five");

            ca.pop_back();
            KSS_ASSERT(ca.size() == 4 && ca[3] == "four");

            ca.push_front("zero");
            string s2("minus one");
            ca.push_front(std::move(s2));
            KSS_ASSERT(ca.size() == 6);
            KSS_ASSERT(ca[0] == "minus one" && ca[1] == "zero");
            KSS_ASSERT(ca[2] == "one" && ca[3] == "two");
            KSS_ASSERT(ca[4] == "three" && ca[5] == "four");

            ca.pop_front();
            KSS_ASSERT(ca.size() == 5 && ca[0] == "zero");

            CircularArray<string> ca1 { "hello", "world" };
            CircularArray<string> caorig(ca);
            CircularArray<string> ca1orig(ca1);
            ca.swap(ca1);
            KSS_ASSERT(ca == ca1orig && ca1 == caorig);
            swap(ca, ca1);
            KSS_ASSERT(ca == caorig && ca1 == ca1orig);

            ca.clear();
            KSS_ASSERT(ca.empty() && ca.size() == 0 && ca.capacity() >= 10);
        }
        {
            // Check that we can go around the circle.
            CircularArray<int> ca { 1, 2, 3, 4, 5 };
            for (int i = 6; i < 50; i += 3) {
                next_values(ca, i, 3, 3);
                KSS_ASSERT(check_values(ca, i-2, 5));
            }
        }
        {
            // Check that we can grow our reserve and go around the circle.
            CircularArray<int> ca;
            int first = 0;
            for (int i = 0; i < 100; i += 5) {
                next_values(ca, i, 5, 1, true);
                ++first;
                KSS_ASSERT(check_values(ca, first, first*4));
            }
        }
    }),
    make_pair("relational operators", [] {
        CircularArray<int> ca { 1, 2, 3, 4, 5 };
        CircularArray<int> caeq(ca);
        CircularArray<int> caless1 { 1, 2, 2, 4, 5 };
        CircularArray<int> caless2 { -1, 2, 3, 4, 5, 6, 7 };
        CircularArray<int> caless3 { 1, 2, 3, 4 };

        KSS_ASSERT(ca == ca);
        KSS_ASSERT(ca == caeq);
        KSS_ASSERT(ca <= caeq);
        KSS_ASSERT(ca >= caeq);
        KSS_ASSERT(ca != caless1);
        KSS_ASSERT(caless1 < ca);
        KSS_ASSERT(caless1 <= ca);
        KSS_ASSERT(ca > caless1);
        KSS_ASSERT(ca >= caless1);
        KSS_ASSERT(caless2 < ca);
        KSS_ASSERT(caless3 < ca);
    })
});
