//
//  stringutil.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2012-12-22.
//  Copyright (c) 2012 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <new>
#include <system_error>

#include "raii.hpp"
#include "stringutil.hpp"

using namespace std;
using namespace kss::util;


string strings::format(string pattern, ...) {
    va_list ap;
    va_start(ap, pattern);
    string s = strings::vformat(pattern, ap);
    va_end(ap);
    return s;
}

namespace {
    string kss_vformat(const char* pattern, va_list ap) {
        assert(pattern != nullptr);

        /* We need to make a copy of ap in case we have to call vsnprintf twice. */
        va_list ap2;
        va_copy(ap2, ap);
        char* buffer = NULL;
        Finally cleanup([&] {
            va_end(ap2);
            if (buffer) { free(buffer); }
        });

        /* First we try a "reasonable" string length. */
        size_t maxlen = strlen(pattern) + 1024;
        buffer = static_cast<char*>(malloc(maxlen));
        if (!buffer) {
            throw bad_alloc();
        }

        int requiredlen = vsnprintf(buffer, maxlen, pattern, ap);
        if (requiredlen < 0) {
            throw system_error(errno, system_category(), "vsnprintf");
        }

        /* If our original length guess was not sufficient, then we reallocate it and do the
         * write a second time.
         */
        if ((size_t)requiredlen >= maxlen) {
            maxlen = (size_t)requiredlen+1;
            char* tmp = buffer;
            buffer = static_cast<char*>(realloc(buffer, maxlen));
            if (!buffer) {
                free(tmp);
                throw bad_alloc();
            }
            vsnprintf(buffer, maxlen, pattern, ap2);
        }

        return string(buffer);
    }
}

string strings::vformat(const string& pattern, va_list ap) {
    return kss_vformat(pattern.c_str(), ap);
}


/* Trimming a string. */
string& strings::ltrim(string& s) noexcept {
    size_t len = s.size();
    size_t i = 0;
    while (i < len && isspace(s[i])) { ++i; }
    if (i > 0) { s.erase(0, i); }
    return s;
}
string& strings::ltrim(string& s, char c) noexcept {
    size_t len = s.size();
    size_t i = 0;
    while (i < len && s[i] == c) { ++i; }
    if (i > 0) { s.erase(0, i); }
    return s;
}

string& strings::rtrim(string& s) noexcept {
    size_t len = s.size();
    size_t i = 0;
    while (i < len && isspace(s[len-i-1])) { ++i; }
    if (i > 0) { s.erase(len-i); }
    return s;
}
string& strings::rtrim(string& s, char c) noexcept {
    size_t len = s.size();
    size_t i = 0;
    while (i < len && s[len-i-1] == c) { ++i; }
    if (i > 0) { s.erase(len-i); }
    return s;
}

namespace {

    bool kss_starts_with(const char* str, const char* prefix) {
        assert(str != nullptr);
        assert(prefix != nullptr);

        /* All strings - even empty ones - start with an empty prefix. */
        size_t plen = strlen(prefix);
        if (!plen) {
            return true;
        }

        /* Empty strings, or strings shorter than the prefix, cannot start with the prefix. */
        size_t slen = strlen(str);
        if (slen < plen) {
            return false;
        }

        /* Compare the start of the string to see if it matches the prefix. */
        return (strncmp(str, prefix, plen) == 0);
    }

    bool kss_ends_with(const char* str, const char* suffix) {
        assert(str != nullptr);
        assert(suffix != nullptr);

        /* All strings - event empty ones - end with an empty suffix. */
        size_t suflen = strlen(suffix);
        if (!suflen) {
            return true;
        }

        /* Empty strings, or strings shorter than the suffix, cannot end with the suffix. */
        size_t slen = strlen(str);
        if (slen < suflen) {
            return false;
        }

        /* Compare the end of the string to see if it matches the suffix. */
        return (strncmp(str+slen-suflen, suffix, suflen) == 0);
    }
}

bool strings::startsWith(const string &str, const string &prefix) noexcept {
    return kss_starts_with(str.c_str(), prefix.c_str());
}

bool strings::endsWith(const string &str, const string &suffix) noexcept {
    return kss_ends_with(str.c_str(), suffix.c_str());
}

string& strings::toUpper(string& s) noexcept {
	transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

wstring& strings::toUpper(wstring& s) noexcept {
	transform(s.begin(), s.end(), s.begin(), ::towupper);
	return s;
}

string& strings::toLower(string& s) noexcept {
	transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

wstring& strings::toLower(wstring& s) noexcept {
	transform(s.begin(), s.end(), s.begin(), ::towlower);
	return s;
}

bool strings::iequal(const string& a, const string& b) noexcept {
    return (icompare(a, b) == 0);
}

bool strings::iequal(const wstring& a, const wstring& b) noexcept {
    return (icompare(a, b) == 0);
}


namespace {
    template <class String, class Fn>
    int _icompare(const String& a, const String& b, Fn fn) noexcept {
        using Int = typename String::traits_type::int_type;
        using SizeT = typename String::size_type;

        // Perform a lexicographical comparison.
        const SizeT alen = a.size();
        const SizeT blen = b.size();
        const SizeT len = std::min(alen, blen);
        for (SizeT i = 0; i < len; ++i) {
            Int diff = fn(Int(a[i])) - fn(Int(b[i]));
            if (diff) { return (int)diff; }
        }

        // At this point we know the strings are equal up to the shortest length. Hence the
        // "greater" one will be the one with the longer length.
        if (alen < blen)        { return -1; }
        else if (alen > blen)   { return 1; }
        else {
            return 0;
        }
    }
}

int strings::icompare(const string& a, const string& b) noexcept {
    return _icompare<string>(a, b, [](int ch){ return std::tolower(ch); });
}

int strings::icompare(const wstring& a, const wstring& b) noexcept {
    return _icompare<wstring>(a, b, [](wint_t ch){ return std::towlower(ch); });
}

