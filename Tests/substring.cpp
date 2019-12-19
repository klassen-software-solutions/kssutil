//
//  substring.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-01-25.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//

#include <iostream>

#include <kss/test/all.h>
#include <kss/util/substring.hpp>

using namespace std;
using namespace kss::util;
using namespace kss::util::strings;
using namespace kss::test;


namespace {
    template <class CHAR>
    class constants {
    public:
        static const CHAR* original_string;
        static const CHAR* is;
        static const CHAR* repl1;
        static const CHAR* after_first_write;
        static const CHAR* second_substr;
        static const CHAR  a;
        static const CHAR* after_second_write;
        static const CHAR* after_third_write;
    };

    template<> const char* constants<char>::original_string = "This is a test of the substring class.";
    template<> const wchar_t* constants<wchar_t>::original_string = L"This is a test of the substring class.";
    template<> const char* constants<char>::is = "is";
    template<> const wchar_t* constants<wchar_t>::is = L"is";
    template<> const char* constants<char>::repl1 = "XXXX";
    template<> const wchar_t* constants<wchar_t>::repl1 = L"XXXX";
    template<> const char* constants<char>::after_first_write = "This XXXX a test of the substring class.";
    template<> const wchar_t* constants<wchar_t>::after_first_write = L"This XXXX a test of the substring class.";
    template<> const char* constants<char>::second_substr = "is a test of the ";
    template<> const wchar_t* constants<wchar_t>::second_substr = L"is a test of the ";
    template<> const char constants<char>::a = 'A';
    template<> const wchar_t constants<wchar_t>::a = L'A';
    template<> const char* constants<char>::after_second_write = "This Asubstring class.";
    template<> const wchar_t* constants<wchar_t>::after_second_write = L"This Asubstring class.";
    template<> const char* constants<char>::after_third_write = "This XXXXsubstring class.";
    template<> const wchar_t* constants<wchar_t>::after_third_write = L"This XXXXsubstring class.";

    template <class CHAR>
    void substring_test_instance() {
        typedef basic_string<CHAR> str;
        typedef SubString<CHAR> substr;

        str s(constants<CHAR>::original_string);
        KSS_ASSERT(substr(s, 5, 2) == constants<CHAR>::is);
        KSS_ASSERT(substr(s, constants<CHAR>::is) == constants<CHAR>::is);

        substr(s, 5, 2) = constants<CHAR>::repl1;
        KSS_ASSERT(s == constants<CHAR>::after_first_write);

        substr(s, constants<CHAR>::repl1) = constants<CHAR>::is;
        KSS_ASSERT(s == constants<CHAR>::original_string);

        substr sub(s, 5, 17);
        KSS_ASSERT(sub == constants<CHAR>::second_substr);
        KSS_ASSERT(sub.size() == 17);
        KSS_ASSERT(!sub.empty());

        sub = constants<CHAR>::a;
        KSS_ASSERT(sub.size() == 1);
        KSS_ASSERT(s == constants<CHAR>::after_second_write);

        sub = constants<CHAR>::repl1;
        KSS_ASSERT(sub.size() == 4);
        KSS_ASSERT(s == constants<CHAR>::after_third_write);
    }
}


static TestSuite ts("strings::substring", {
    make_pair("basic_substring<char>", substring_test_instance<char>),
    make_pair("basic_substring<wchar_t>", substring_test_instance<wchar_t>)
});
