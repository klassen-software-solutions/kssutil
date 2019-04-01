//
//  stringutil.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2012-12-21.
//  Copyright (c) 2012 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

/*!
 \file
 \brief Miscellaneous algorithms related to strings.
 */

#ifndef kssutil_stringutil_hpp
#define kssutil_stringutil_hpp

#include <string>

namespace kss { namespace util { namespace strings {

    /*!
     Perform printf style formatting and return the resulting string.
     */
    std::string format(std::string pattern, ...);
    std::string vformat(const std::string& pattern, va_list ap);

    /*!
     Trim whitespace or a specific repeating character from a string. The modified
     string is also returned. Note that there are versions to trim only the left (l),
     the right (r) or both.
     @return the modified string.
     */
    std::string& ltrim(std::string& s) noexcept;
    std::string& ltrim(std::string& s, char c) noexcept;
    std::string& rtrim(std::string& s) noexcept;
    std::string& rtrim(std::string& s, char c) noexcept;
    inline std::string& trim(std::string& s) noexcept { return ltrim(rtrim(s)); }
    inline std::string& trim(std::string& s, char c) noexcept { return ltrim(rtrim(s, c), c); }

    /*!
     Determine if a string starts or ends with another string.
     */
    bool startsWith(const std::string& str, const std::string& prefix) noexcept;
    bool endsWith(const std::string& str, const std::string& suffix) noexcept;

    /*!
     Convert strings to upper or lowercase. This can be done either in place or
     by creating new strings.
     @return either a reference to the, now modified, string or a new string.
     @throws std::bad_alloc if a new string could not be allocated.
     */
    std::string& toUpper(std::string& s) noexcept;
    inline std::string toUpper(const std::string& s) {
        std::string s2(s);
        return toUpper(s2);
    }

    std::wstring& toUpper(std::wstring& s) noexcept;
    inline std::wstring toUpper(const std::wstring& s) {
        std::wstring s2(s);
        return toUpper(s2);
    }

    std::string& toLower(std::string& s) noexcept;
    inline std::string toLower(const std::string& s) {
        std::string s2(s);
        return toLower(s2);
    }

    std::wstring& toLower(std::wstring& s) noexcept;
    inline std::wstring toLower(const std::wstring& s) {
        std::wstring s2(s);
        return toLower(s2);
    }


    /*!
     Case insensitive string comparison.
     @return true (iequal) if the two string are equal except for case
     @return <0,0,or >0 (icompare) based on the string comparison. (i.e. like strcmp)
     */
    bool iequal(const std::string& a, const std::string& b) noexcept;
    bool iequal(const std::wstring& a, const std::wstring& b) noexcept;
    int icompare(const std::string& a, const std::string& b) noexcept;
    int icompare(const std::wstring& a, const std::wstring& b) noexcept;


    /*!
     Count the number of times a substring occurs in a source string. This should
     work on any object that follows the basic_string API.

     This is based on code found at:
     https://stackoverflow.com/questions/6067460/c-how-to-calculate-the-number-time-a-string-occurs-in-a-data

     @param source The string being searched.
     @param substr The sub-string we are looking for.
     @param allowOverlaps If true then overlapping substrings are counted separately.
     @return The number of times the substring is found.
     */
    template <class Char, class traits = std::char_traits<Char>, class Alloc = std::allocator<Char>>
    unsigned countOccurrencesOf(const std::basic_string<Char, traits, Alloc>& source,
                                const std::basic_string<Char, traits, Alloc>& substr,
                                bool allowOverlaps = false) noexcept
    {
        using string_t = std::basic_string<Char, traits, Alloc>;
        const typename string_t::size_type step = (allowOverlaps ? 1 : substr.size());
        unsigned count = 0;
        typename string_t::size_type pos = 0;

        while ((pos = source.find(substr, pos)) != string_t::npos) {
            pos += step;
            ++count;
        }

        return count;
    }

    template <class Char, class Traits = std::char_traits<Char>, class Alloc = std::allocator<Char>>
    inline unsigned countOccurrencesOf(const std::basic_string<Char, Traits, Alloc>& source,
                                       const Char* substr,
                                       bool allowOverlaps = false) noexcept
    {
        using string_t = std::basic_string<Char, Traits, Alloc>;
        return countOccurrencesOf(source, string_t(substr), allowOverlaps);
    }
}}}

#endif
