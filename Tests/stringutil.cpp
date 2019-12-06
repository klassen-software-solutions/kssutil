//
//  stringutil.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2012-12-22.
//  Copyright (c) 2012 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/util/stringutil.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::util;
using namespace kss::util::strings;
using namespace kss::test;


static TestSuite ts("strings::stringutil", {
    make_pair("basic tests", [] {
        // Test the formatting.
        KSS_ASSERT(isEqualTo<string>("This is test number 5.0", [] {
            const auto s = format("%s is test number %.1f", "This", 5.F);
            return format("%s is test number %.1f", "This", 5.F);
        }));

        // Test the trimming.
        string s = "  This is a test of whitespace trimming.   ";
        KSS_ASSERT(ltrim(s) == "This is a test of whitespace trimming.   ");
        s = "  This is a test of whitespace trimming.   ";
        KSS_ASSERT(rtrim(s) == "  This is a test of whitespace trimming.");
        s = "  This is a test of whitespace trimming.   ";
        KSS_ASSERT(trim(s) == "This is a test of whitespace trimming.");
        s = "No trimming this time.";
        KSS_ASSERT(ltrim(s) == "No trimming this time.");
        KSS_ASSERT(rtrim(s) == "No trimming this time.");
        KSS_ASSERT(trim(s) == "No trimming this time.");

        s = "tttThis is a test of non-whitespace trimming....";
        KSS_ASSERT(ltrim(s, 't') == "This is a test of non-whitespace trimming....");
        KSS_ASSERT(ltrim(s, 'x') == "This is a test of non-whitespace trimming....");
        KSS_ASSERT(rtrim(s, '.') == "This is a test of non-whitespace trimming");
        KSS_ASSERT(rtrim(s, 'x') == "This is a test of non-whitespace trimming");
        s = "...This is a test of non-whitespace trimming....";
        KSS_ASSERT(trim(s, '.') == "This is a test of non-whitespace trimming");
        KSS_ASSERT(trim(s, 'g') == "This is a test of non-whitespace trimmin");
        KSS_ASSERT(trim(s, 'x') == "This is a test of non-whitespace trimmin");

        // Test the prefix and suffix.
        KSS_ASSERT(startsWith("this is the string", "this is"));
        KSS_ASSERT(startsWith("this is the string", string()));
        KSS_ASSERT(startsWith("this is the string", "t"));
        KSS_ASSERT(startsWith(string(), string()));
        KSS_ASSERT(!startsWith("this is the string", "x"));
        KSS_ASSERT(!startsWith("this is the string", "this is not"));
        KSS_ASSERT(!startsWith("t", "this"));
        KSS_ASSERT(!startsWith(string(), "hi"));

        KSS_ASSERT(endsWith("this is the string", "e string"));
        KSS_ASSERT(endsWith("this is the string", ""));
        KSS_ASSERT(endsWith("this is the string", "g"));
        KSS_ASSERT(endsWith("", ""));
        KSS_ASSERT(!endsWith("this is the string", "x"));
        KSS_ASSERT(!endsWith("this is the string", "strong"));
        KSS_ASSERT(!endsWith("t", "out"));
        KSS_ASSERT(!endsWith("", "hi"));
    }),
    make_pair("case conversion and comparison", [] {
        {
            string s1 = "this is a test";
            string s2 = "ThiS IS a tEst";
            string s3 = "a string";
            string s4 = "XXXX";
            string s5 = "THIS IS";

            KSS_ASSERT(iequal(s1, s2));
            KSS_ASSERT(iequal(s1, s1));
            KSS_ASSERT(!iequal(s1, s3));
            KSS_ASSERT(icompare(s1, s2) == 0);
            KSS_ASSERT(icompare(s1, s3) > 0);
            KSS_ASSERT(icompare(s3, s1) < 0);
            KSS_ASSERT(icompare(s1, s4) < 0);
            KSS_ASSERT(icompare(s4, s1) > 0);
            KSS_ASSERT(icompare(s1, s5) > 0);
            KSS_ASSERT(icompare(s5, s1) < 0);

            string s = s1;
            KSS_ASSERT(toUpper(s) == "THIS IS A TEST");
            KSS_ASSERT(toUpper((const string&)s1) == "THIS IS A TEST");
            KSS_ASSERT(s1 == "this is a test");

            s = s2;
            KSS_ASSERT(toLower(s) == "this is a test");
            KSS_ASSERT(toLower((const string&)s2) == "this is a test");
            KSS_ASSERT(s2 == "ThiS IS a tEst");
        }
        {
            wstring s1 = L"this is a test";
            wstring s2 = L"ThiS IS a tEst";
            wstring s3 = L"a string";
            wstring s4 = L"XXXX";
            wstring s5 = L"THIS IS";

            KSS_ASSERT(iequal(s1, s2));
            KSS_ASSERT(iequal(s1, s1));
            KSS_ASSERT(!iequal(s1, s3));
            KSS_ASSERT(icompare(s1, s2) == 0);
            KSS_ASSERT(icompare(s1, s3) > 0);
            KSS_ASSERT(icompare(s3, s1) < 0);
            KSS_ASSERT(icompare(s1, s4) < 0);
            KSS_ASSERT(icompare(s4, s1) > 0);
            KSS_ASSERT(icompare(s1, s5) > 0);
            KSS_ASSERT(icompare(s5, s1) < 0);

            wstring s = s1;
            KSS_ASSERT(toUpper(s) == L"THIS IS A TEST");
            KSS_ASSERT(toUpper((const wstring&)s1) == L"THIS IS A TEST");
            KSS_ASSERT(s1 == L"this is a test");

            s = s2;
            KSS_ASSERT(toLower(s) == L"this is a test");
            KSS_ASSERT(toLower((const wstring&)s2) == L"this is a test");
            KSS_ASSERT(s2 == L"ThiS IS a tEst");
        }
    }),
    make_pair("countOccurrencesOf", [] {
        const string s = "This is a test of AAAAAAAA substring counting seAArch.";
        KSS_ASSERT(strings::countOccurrencesOf(s, "AA") == 5);
        KSS_ASSERT(strings::countOccurrencesOf(s, "AA", true) == 8);
        KSS_ASSERT(strings::countOccurrencesOf(s, "XX") == 0);
        KSS_ASSERT(strings::countOccurrencesOf(s, "XX", true) == 0);

        const wstring ws = L"This is a test of AAAAAAAA substring counting seAArch.";
        KSS_ASSERT(strings::countOccurrencesOf(ws, L"AA") == 5);
        KSS_ASSERT(strings::countOccurrencesOf(ws, L"AA", true) == 8);
        KSS_ASSERT(strings::countOccurrencesOf(ws, L"XX") == 0);
        KSS_ASSERT(strings::countOccurrencesOf(ws, L"XX", true) == 0);
    })
});
