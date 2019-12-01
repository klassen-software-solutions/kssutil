

/*! \file      substring.hpp
   \author     Steven W. Klassen (klassens@acm.org)
   \date       Sat Jun 14 2003
   \brief      Write-through substring.

 This file is Copyright (c) 2003 by Klassen Software Solutions.
 Licensing follows the MIT License.
 */


#ifndef kssutil_substring_hpp
#define kssutil_substring_hpp

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>

#include <kss/contract/all.h>

#include "add_rel_ops.hpp"

namespace kss { namespace util { namespace strings {

    /*!
     \brief Write-through substring.

     The SubString class provides writable access to a substring
     of another string.  Note that since this is backed by the original
     string, the substring will become invalid when the original string
     goes out of scope.

     Note that if a SubString constructor results in an empty
     substring (for example the search fails), then any attempt to
     modify the substring will result in an out_of_range exception.

     While the code in this class is original, it is motivated by the
     Basic_substring class described in _THE C++ PROGRAMMING LANGUAGE,
     SPECIAL EDITION_ by Stroustrup, ISBN 0-201-88954-4.
     */
    template <class Char,
              class Traits = std::char_traits<Char>,
              class Alloc = std::allocator<Char>>
    class SubString : public kss::util::AddRelOps<SubString<Char, Traits, Alloc>> {
    public:

        using size_type = typename std::basic_string<Char>::size_type;
        using allocator_type = Alloc;
        using pointer = typename Alloc::pointer;
        using const_pointer = typename Alloc::const_pointer;
        using value_type = Char;

        /*!
         Create an n-character substring from str, starting with position
         i.  Note that if n goes beyond the end of str, it will be trimmed
         to the final position in str.
         */
        SubString(std::basic_string<Char, Traits, Alloc>& str,
                  size_type i,
                  size_type n = std::basic_string<Char, Traits, Alloc>::npos)
        : _str(str), _allocator(str.get_allocator()), _cstr(nullptr)
        {
            size_type len = str.length();
            _i = (i >= len ? std::basic_string<Char, Traits, Alloc>::npos : i);
            _n = std::min(n, len - _i);

            constexpr auto npos = std::basic_string<Char, Traits, Alloc>::npos;
            kss::contract::postconditions({
                KSS_EXPR(_i == i || i == npos),
                KSS_EXPR(_n <= n),
                KSS_EXPR(_cstr == nullptr)
            });
        }

        /*!
         Search for the first occurrence of str2 in str, starting at
         position i, and create the substring from it.  (Substring is
         created based on str, not on str2.)
         */
        SubString(std::basic_string<Char, Traits, Alloc>& str,
                  const std::basic_string<Char, Traits, Alloc>& str2,
                  size_type i = 0)
        : _str(str), _allocator(str.get_allocator()), _cstr(nullptr)
        {
            _i = str.find(str2, i);
            if (_i != std::basic_string<Char, Traits, Alloc>::npos) {
                _n = str2.length();
            }

            kss::contract::postconditions({
                KSS_EXPR(_i >= i),
                KSS_EXPR(_n == str2.length() || _n == 0),
                KSS_EXPR(_cstr == nullptr)
            });
        }

        /*!
         Destroy the substring.
         */
        ~SubString() noexcept {
            freeCStr();
        }

        /*!
         Replace the substring (in the original string) with str.
         @throws std::out_of_range if the substring is not valid
         */
        SubString& operator=(const std::basic_string<Char, Traits, Alloc>& str) {
            return assign(str.data(), str.length());
        }

        /*!
         Replace the substring (in the original string) with str.
         @throws std::out_of_range if the substring is not valid
         */
        SubString& operator=(const SubString& str) {
            return assign(str.data(), _n);
        }

        /*!
         Replace the substring (in the original string) with the NULL-
         terminated characters starting with cptr.
         @throws std::invalid_argument if cptr is NULL
         @throws std::out_of_range if the substring is not valid
         */
        SubString& operator=(const_pointer cptr) {
            return assign(cptr, std::char_traits<Char>::length(cptr));
        }

        /*!
         Replace the substring (in the original string) with the single
         character ch.
         @throws std::out_of_range if the substring is currently empty
         */
        SubString& operator=(Char ch) {
            return assign(&ch, size_type(1));
        }

        /*!
         Allow a substring to be cast to a string.  This results in a
         copy of the substring characters.  If the substring is currently
         empty, an empty but valid string will be returned.
         */
        operator std::basic_string<Char, Traits, Alloc>() const {
            if (_i >= _str.length() || _n == 0) {
                return std::basic_string<Char, Traits, Alloc>();
            }
            else {
                return std::basic_string<Char, Traits, Alloc>(_str, _i, _n, _allocator);
            }
        }

        /*!
         Return a NULL-terminated "C" style string built from the substring.
         Note that this results in the characters being copied.  If the
         substring is currently empty, an empty but valid "C" string will
         be returned.  Also note that the memory allocated by this method
         will become invalid once the substring goes out of scope.  If we
         cannot allocate enough memory a bad_alloc exception is thrown.
         The memory will be built using the allocator from the original
         string.
         @throws std::bad_alloc if memory could not be allocated
         */
        const_pointer c_str() const {
            if (_cstr == nullptr && _i < _str.length()) {
                // may already be copied, if not make the copy
                SubString<Char, Traits, Alloc>* obj = const_cast<SubString<Char, Traits, Alloc>*>(this);
                obj->_cstr = obj->_allocator.allocate(_n + 1);
                if (_n > 0) {
                    std::uninitialized_copy(data(), data()+_n, obj->_cstr);
                }
                obj->_allocator.construct(obj->_cstr+_n, Char(0));
            }
            return _cstr;
        }

        /*!
         Return a pointer to the start of the substring characters.  No
         guarantee is made regarding a NULL terminator hence you must
         use the size() method to determine the proper end of the
         characters.  Unlike c_str() this requires no memory allocation.
         */
        const_pointer data() const noexcept {
            return (_i >= _str.length() ? nullptr : _str.data() + _i);
        }

        /*!
         Returns the size of the substring.
         */
        size_type size() const noexcept { return _n; }

        /*!
         Returns true if the substring is empty and false otherwise.
         */
        bool empty() const noexcept { return (_n == 0); }

        /*!
         Comparators. The "missing" operators are added by the AddRelOps subclassing.
         */
        bool operator==(const SubString& rhs) noexcept {
            if (this == &rhs) {
                return true;
            }
            if (size() != rhs.size()) {
                return false;
            }
            return (Traits::compare(data(), rhs.data(), size()) == 0);
        }

        bool operator<(const SubString& rhs) noexcept {
            if (this == &rhs) {
                return false;
            }
            size_type n = std::min(size(), rhs.size());
            int retval = Traits::compare(data(), rhs.data(), n);
            if (retval < 0) {
                return true;
            }
            else if (retval > 0) {
                return false;
            }
            return (size() < rhs.size());
        }

    private:
        std::basic_string<Char>&    _str;
        size_type                   _i = std::basic_string<Char, Traits, Alloc>::npos;
        size_type                   _n = 0;
        Alloc                       _allocator;
        pointer                     _cstr = nullptr;

        void freeCStr() {
            if (_cstr != nullptr) {
                for (pointer p = _cstr; p < _cstr+_n+1; ++p) {
                    _allocator.destroy(p);
                }
                _allocator.deallocate(_cstr, _n+1);
                _cstr = nullptr;
            }

            kss::contract::postconditions({
                KSS_EXPR(_cstr == nullptr)
            });
        }

        SubString& assign(const const_pointer ptr, size_type new_n) {
            kss::contract::preconditions({
                KSS_EXPR(_cstr == nullptr)
            });

            if (_i >= _str.length()) {
                throw std::out_of_range("Substring is out of range of the original.");
            }

            freeCStr();
            _str.replace(_i, _n, ptr, new_n);
            _n = new_n;
            return *this;
        }
    };

    /*!
     Compare a substring to a string of equivalent type.
     */
    template <class Char, class Traits, class Alloc>
    bool operator==(const SubString<Char, Traits, Alloc>& lhs,
                    const std::basic_string<Char, Traits, Alloc>& rhs) noexcept
    {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        return (Traits::compare(lhs.data(), rhs.data(), lhs.size()) == 0);
    }

    template <class Char, class Traits, class Alloc>
    inline bool operator==(const std::basic_string<Char, Traits, Alloc>& lhs,
                           const SubString<Char, Traits, Alloc>& rhs) noexcept
    {
        return (rhs == lhs);
    }

    template <class Char, class Traits, class Alloc>
    bool operator<(const SubString<Char, Traits, Alloc>& lhs,
                   const std::basic_string<Char, Traits, Alloc>& rhs) noexcept
    {
        typename SubString<Char, Traits, Alloc>::size_type n = std::min(lhs.size(), rhs.size());
        int retval = Traits::compare(lhs.data(), rhs.data(), n);
        if (retval < 0) {
            return true;
        }
        else if (retval > 0) {
            return false;
        }
        return (lhs.size() < rhs.size());
    }

    template <class Char, class Traits, class Alloc>
    bool operator<(const std::basic_string<Char, Traits, Alloc>& lhs,
                   const SubString<Char, Traits, Alloc>& rhs) noexcept
    {
        typename SubString<Char, Traits, Alloc>::size_type n = std::min(lhs.size(), rhs.size());
        int retval = Traits::compare(lhs.data(), rhs.data(), n);
        if (retval < 0) {
            return true;
        }
        else if (retval > 0) {
            return false;
        }
        return (lhs.size() < rhs.size());
    }

    /*!
     Compare a substring to a const pointer of equivalent type.  Note that
     the pointer must be NULL-terminated.
     */
    template <class Char, class Traits, class Alloc>
    bool operator==(const SubString<Char, Traits, Alloc>& lhs,
                    typename SubString<Char, Traits, Alloc>::const_pointer rhs) noexcept
    {
        typename SubString<Char, Traits, Alloc>::size_type rhsSize =
        Traits::length(rhs);
        if (lhs.size() != rhsSize) {
            return false;
        }
        return (Traits::compare(lhs.data(), rhs, rhsSize) == 0);
    }

    template <class Char, class Traits, class Alloc>
    bool operator==(typename SubString<Char, Traits, Alloc>::const_pointer lhs,
                    const SubString<Char, Traits, Alloc>& rhs) noexcept
    {
        return (rhs == lhs);
    }

    template <class Char, class Traits, class Alloc>
    bool operator<(const SubString<Char, Traits, Alloc>& lhs,
                   typename SubString<Char, Traits, Alloc>::const_pointer rhs) noexcept
    {
        typename SubString<Char, Traits, Alloc>::size_type rhsSize = Traits::length(rhs);
        typename SubString<Char, Traits, Alloc>::size_type n =
        std::min(lhs.size(), rhsSize);
        int retval = Traits::compare(lhs.data(), rhs, n);
        if (retval < 0) {
            return true;
        }
        else if (retval > 0) {
            return false;
        }
        return (lhs.size() < rhsSize);
    }

    template <class Char, class Traits, class Alloc>
    bool operator<(typename SubString<Char, Traits, Alloc>::const_pointer lhs,
                   const SubString<Char, Traits, Alloc>& rhs) noexcept
    {
        typename SubString<Char, Traits, Alloc>::size_type rhsSize = Traits::length(rhs);
        typename SubString<Char, Traits, Alloc>::size_type n = std::min(lhs.size(), rhsSize);
        int retval = Traits::compare(lhs, rhs.data(), n);
        if (retval < 0) {
            return true;
        }
        else if (retval > 0) {
            return false;
        }
        return (lhs.size() < rhsSize);
    }


    /*!
     Shorthand for a substring of an std::string.
     */
    typedef SubString<char, std::char_traits<char>, std::allocator<char> > substring_t;

    /*!
     Shorthand for a substring of an std::wstring.
     */
    typedef SubString<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > wsubstring_t;


}}}

#endif
