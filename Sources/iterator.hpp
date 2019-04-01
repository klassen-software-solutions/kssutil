//
//  iterator.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-10-24.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//
// This file provides iterator implementations based on common operations that need to
// be provided by the container classes. In most cases there are two template types that
// must be provided: Container which specifies the container class which provides the
// necessary operations, and T which specifies the type of data that the iterator
// provides access to.
//
//  Licensing follows the MIT License.
//

/*!
 \file
 \brief Base classes to easily generate custom iterators.
 */


#ifndef kssutil_iterator_hpp
#define kssutil_iterator_hpp

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#include "add_rel_ops.hpp"
#include "utility.hpp"

namespace kss { namespace util { namespace iterators {

    /*!
     \brief Base implementation of a forward iterator.

     The ForwardIterator class provides a forward iterator for any class that can
     move through its elements in a single direction. Specifically it needs to support
     the following operations:

     @code
     void hasAnother(); // Returns true if there is at least one more item available.
     void next(T& t);   // Fill in the next value .
     @endcode

     The type T needs to support a default constructor and copy and move semantics.

     Typically the container begin() and end() methods would be defined along the
     lines of the following:

     @code
     using iterator = kss::util::iterator::ForwardIterator<Container, T>;
     iterator begin() { return iterator(*this); }
     iterator end() { return iterator(); }
     @endcode
     */
    template <typename Container, typename T>
    class ForwardIterator
    : public std::iterator<std::forward_iterator_tag, T, ptrdiff_t, const T*, const T&>
    {
    public:

        /*!
         The default constructor creates an iterator that points to nothing other
         than a special "end" flag.

         The container version of the constructor creates an iterator and attempts
         to obtain the next value from the container.

         @throws any exception that next() may throw
         @throws any exception that the T copy and move semantics may throw
         */
        ForwardIterator() : _cont(nullptr) {}
        explicit ForwardIterator(Container& cont) {
            if (cont.hasAnother()) {
                _cont = &cont;
                operator++();
            }
            else {
                _cont = nullptr;
            }
        }

        ForwardIterator(const ForwardIterator& it) = default;
        ForwardIterator(ForwardIterator&& it)
        : _cont(it._cont), _value(std::move(it._value))
        {
            it._cont = nullptr;
        }

        ForwardIterator& operator=(const ForwardIterator& it) = default;
        ForwardIterator& operator=(ForwardIterator&& it) noexcept {
            if (&it != this) {
                _cont = it._cont;
                _value = std::move(it._value);
            }
            return *this;
        }

        /*!
         Determine iterator equality. Any "end" iterators are considered equal, even if
         they have different streams. Any other iterators are equal so long as they
         are referencing the same stream. And iterator that is not an "end" iterator
         is not equal to an "end" iterator.
         */
        bool operator==(const ForwardIterator& it) const noexcept {
            if (&it == this) { return true; }
            if (_cont == it._cont) { return true; }
            return false;
        }
        inline bool operator!=(const ForwardIterator& it) const noexcept {
            return !operator==(it);
        }

        /*!
         Dereference the iterator. This is undefined if we try to dereference the end()
         value.
         */
        const T& operator*() const { return _value; }
        const T* operator->() const { return &_value; }

        /*!
         Increment the iterator. If the containers eof() is true then this will become
         an instance of the end iterator. Otherwise we call the containers >> operator
         and store the result.

         If for some reason eof() cannot be known until operator>> is called, then you
         should have your operator>> throw a kss::eof exception if it reaches the end.
         This allows us to avoid having to write "lookahead" code in these situations.

         @throws runtime_error if this is already the end iterator.
         @throws any exception that container::eof() or container::operator>>(T&) may throw
         */
        ForwardIterator& operator++() {
            // preconditions
            if (!_cont) {
                _KSSUTIL_PRECONDITIONS_FAILED
            }

            if (_cont->hasAnother()) {
                _cont->next(_value);
            }
            else {
                _cont = nullptr;
            }

            return *this;
        }
        ForwardIterator operator++(int) {
            ForwardIterator it = *this;
            operator++();
            return it;
        }

        /*!
         Swap with another iterator.
         */
        void swap(ForwardIterator& b) noexcept {
            if (this != &b) {
                std::swap(_cont, b._cont);
                std::swap(_value, b._value);
            }
        }

    private:
        Container*  _cont;
        T           _value;
    };


    /*!
     \brief Base implementation of a random access iterator.
     
     The RandomAccessIterator class provides a random access iterator for any container
     that allows random access via the following operations. Note that the non-const
     operator is needed for RandomAccessIterator.

     @code
     T& operator[](size_t);                // not needed in the const version
     const T& operator[](size_t) const;    // not needed in the non-const version
     size_t size() const;
     @endcode

     Typically the containers begin() and end() methods would be defined as follows:

     @code
     using iterator = kss::util::iterators::RandomAccessIterator<Container>;
     using const_iterator = kss::util::iterators::ConstRandomAccessIterator<Container>;
     iterator begin() noexcept { return iterator(*this); }
     iterator end() noexcept { return iterator(*this, true); }
     const_iterator begin() const noexcept { return const_iterator(*this); }
     const_iterator end() const noexcept { return const_iterator(*this, true); }
     @endcode

     The type T needs to support a default constructor and copy semantics.
     */
    template <typename Container>
    class RandomAccessIterator
    : public kss::util::AddRelOps<RandomAccessIterator<Container>>,
      public std::iterator<
        std::random_access_iterator_tag,
        typename Container::value_type,
        typename Container::difference_type,
        typename Container::pointer,
        typename Container::reference >
    {
    public:
        RandomAccessIterator() = default;

        explicit RandomAccessIterator(Container& container, bool isEnd = false)
        : _container(&container), _pos(isEnd ? container.size() : 0)
        {}

        RandomAccessIterator(const RandomAccessIterator&) = default;
        ~RandomAccessIterator() noexcept = default;
        RandomAccessIterator& operator=(const RandomAccessIterator& it) noexcept = default;

        // Two iterators are considered equal if they are both pointing to the same position
        // of the same container. The inequality comparisons only check the positions, they do
        // not check the container. Note that the "missing" comparison operators are added
        // via the AddRelOps declaration.
        bool operator==(const RandomAccessIterator& rhs) const noexcept {
            return (_container == rhs._container && _pos == rhs._pos);
        }
        bool operator<(const RandomAccessIterator& rhs) const noexcept {
            return (_pos < rhs._pos);
        }

        // Dereference the iterators. Note that to obtain maximum efficiency (since this
        // is expected to be done a lot) we do not check if _container and _pos are
        // valid. If they are not, you can expect a SEGV or something similarly bad
        // to happen.
        typename Container::reference operator*()                       { return (*_container)[_pos]; }
        const typename Container::reference operator*() const           { return (*_container)[_pos]; }
        typename Container::pointer operator->()                        { return &(*_container)[_pos]; }
        const typename Container::pointer operator->() const            { return &(*_container)[_pos]; }
        typename Container::reference operator[](size_t i)              { return (*_container)[_pos+i]; }
        const typename Container::reference operator[](size_t i) const  { return (*_container)[_pos+i]; }

        // Pointer arithmetic. For efficiency reasons we do not check if the resulting
        // position is valid.
        RandomAccessIterator& operator++() noexcept   { ++_pos; return *this; }
        RandomAccessIterator operator++(int) noexcept {
            RandomAccessIterator tmp(*this);
            operator++();
            return tmp;
        }
        RandomAccessIterator& operator+=(typename Container::difference_type n) noexcept {
            (n >= 0 ? _pos += size_t(n) : _pos -= size_t(-n));
            return *this;
        }
        RandomAccessIterator operator+(typename Container::difference_type n) const noexcept {
            RandomAccessIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        RandomAccessIterator& operator--() noexcept   { --_pos; return *this; }
        RandomAccessIterator& operator-=(typename Container::difference_type n) noexcept {
            (n >= 0 ? _pos -= size_t(n) : _pos += size_t(-n));
            return *this;
        }
        RandomAccessIterator operator-(typename Container::difference_type n) const noexcept {
            RandomAccessIterator tmp(*this);
            tmp -= n;
            return tmp;
        }
        typename Container::difference_type operator-(const RandomAccessIterator& rhs) const noexcept {
            return typename Container::difference_type(_pos >= rhs._pos ? _pos - rhs._pos : -(rhs._pos - _pos));
        }

        /*!
         Swap with another iterator.
         */
        void swap(RandomAccessIterator& b) noexcept {
            if (this != &b) {
                std::swap(_container, b._container);
                std::swap(_pos, b._pos);
            }
        }

    private:
        Container*  _container = nullptr;
        size_t      _pos = 0;
    };

    template <typename Container>
    inline RandomAccessIterator<Container> operator+(typename Container::difference_type n,
                                                     const RandomAccessIterator<Container>& c) noexcept
    {
        return (c + n);
    }


    /*!
     \brief Base implementation of a const version of a random access iterator.

     Const version of the random access iterator. The comments for RandomAccessIterator
     follow for this one as well.
     */
    template <typename Container>
    class ConstRandomAccessIterator
    : public kss::util::AddRelOps<ConstRandomAccessIterator<Container>>,
      public std::iterator<
        std::random_access_iterator_tag,
        typename Container::value_type,
        typename Container::difference_type,
        typename Container::const_pointer,
        typename Container::const_reference >
    {
    public:
        ConstRandomAccessIterator() = default;

        explicit ConstRandomAccessIterator(const Container& container, bool isEnd = false)
        : _container(&container), _pos(isEnd ? container.size() : 0)
        {}

        ConstRandomAccessIterator(const ConstRandomAccessIterator&) = default;
        ~ConstRandomAccessIterator() noexcept = default;
        ConstRandomAccessIterator& operator=(const ConstRandomAccessIterator& it) = default;

        // Two iterators are considered equal if they are both pointing to the same position
        // of the same container. The inequality comparisons only check the positions, they do
        // not check the container. Note that the "missing" comparison operators are added
        // via the AddRelOps declaration.
        bool operator==(const ConstRandomAccessIterator& rhs) const noexcept {
            return (_container == rhs._container && _pos == rhs._pos);
        }
        bool operator<(const ConstRandomAccessIterator& rhs) const noexcept {
            return (_pos < rhs._pos);
        }

        // Dereference the iterators. Note that to obtain maximum efficiency (since this
        // is expected to be done a lot) we do not check if _container and _pos are
        // valid. If they are not, you can expect a SEGV or something similarly bad
        // to happen.
        typename Container::const_reference operator*() const           { return (*_container)[_pos]; }
        typename Container::const_pointer operator->() const            { return &(*_container)[_pos]; }
        typename Container::const_reference operator[](size_t i) const  { return (*_container)[_pos+i]; }

        // Pointer arithmetic. For efficiency reasons we do not check if the resulting
        // position is valid.
        ConstRandomAccessIterator& operator++() noexcept { ++_pos; return *this; }
        ConstRandomAccessIterator operator++(int) noexcept {
            ConstRandomAccessIterator tmp(*this);
            operator++();
            return tmp;
        }
        ConstRandomAccessIterator& operator+=(typename Container::difference_type n) noexcept {
            (n >= 0 ? _pos += size_t(n) : _pos -= size_t(-n));
            return *this;
        }
        ConstRandomAccessIterator operator+(typename Container::difference_type n) const noexcept {
            ConstRandomAccessIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        ConstRandomAccessIterator& operator--() noexcept { --_pos; return *this; }
        ConstRandomAccessIterator operator--(int) noexcept {
            ConstRandomAccessIterator tmp(_pos);
            operator--();
            return tmp;
        }
        ConstRandomAccessIterator& operator-=(typename Container::difference_type n) noexcept {
            (n >= 0 ? _pos -= size_t(n) : _pos += size_t(-n));
            return *this;
        }
        ConstRandomAccessIterator operator-(typename Container::difference_type n) const noexcept {
            ConstRandomAccessIterator tmp(*this);
            tmp -= n;
            return tmp;
        }
        typename Container::difference_type operator-(const ConstRandomAccessIterator& rhs) const noexcept {
            return typename Container::difference_type(_pos >= rhs._pos ? _pos - rhs._pos : -(rhs._pos - _pos));
        }

        /*!
         Swap with another iterator.
         */
        void swap(ConstRandomAccessIterator& b) noexcept {
            if (this != &b) {
                std::swap(_container, b._container);
                std::swap(_pos, b._pos);
            }
        }

    private:
        const Container*    _container = nullptr;
        size_t              _pos = 0;
    };

    template <typename Container>
    ConstRandomAccessIterator<Container> operator+(typename Container::difference_type n, const ConstRandomAccessIterator<Container>& c) noexcept {
        return (c + n);
    }


    /*!
     \brief Base implementation of a random access iterator that generate elements as needed.
     
     Copy version of the random access iterator. The comments for RandomAccessIterator
     follow for this one as well.

     This iterator is for containers that don't necessarily contain their elements but
     may generate them on an as-needed basis. Specifically if operator[] returns a copy
     of an element instead of a reference to one, then this iterator will be needed.

     Note that this iterator is virtually the same as the ConstRandomAccessIterator
     except that the pointer type will be a std::shared_ptr<const value_type> and
     there is no reference type.
     */
    template <typename Container>
    class CopyRandomAccessIterator
    : public kss::util::AddRelOps<CopyRandomAccessIterator<Container>>,
      public std::iterator<
        std::random_access_iterator_tag,
        typename Container::value_type,
        typename Container::difference_type,
        std::shared_ptr<const typename Container::value_type>,
        void >
    {
    public:
        CopyRandomAccessIterator() = default;

        explicit CopyRandomAccessIterator(const Container& container, bool isEnd = false)
        : _container(&container), _pos(isEnd ? container.size() : 0)
        {}

        CopyRandomAccessIterator(const CopyRandomAccessIterator& rhs) = default;
        ~CopyRandomAccessIterator() noexcept = default;
        CopyRandomAccessIterator& operator=(const CopyRandomAccessIterator&) = default;

        // Two iterators are considered equal if they are both pointing to the same position
        // of the same container. The inequality comparisons only check the positions, they do
        // not check the container. Note that the "missing" comparison operators are added
        // via the AddRelOps declaration.
        bool operator==(const CopyRandomAccessIterator& rhs) const noexcept {
            return (_container == rhs._container && _pos == rhs._pos);
        }
        bool operator<(const CopyRandomAccessIterator& rhs) const noexcept {
            return (_pos < rhs._pos);
        }

        // Dereference the iterators. Note that to obtain maximum efficiency (since this
        // is expected to be done a lot) we do not check if _container and _pos are
        // valid. If they are not, you can expect a SEGV or something similarly bad
        // to happen.
        typename Container::value_type operator*() const { return (*_container)[_pos]; }
        std::shared_ptr<const typename Container::value_type> operator->() const {
            return std::shared_ptr<const typename Container::value_type>(new typename Container::value_type((*_container)[_pos]));
        }
        typename Container::value_type operator[](size_t i) const {
            return (*_container)[_pos+i];
        }

        // Pointer arithmetic. For efficiency reasons we do not check if the resulting
        // position is valid.
        CopyRandomAccessIterator& operator++() noexcept { ++_pos; return *this; }
        CopyRandomAccessIterator operator++(int) noexcept {
            CopyRandomAccessIterator tmp(*this);
            operator++();
            return tmp;
        }
        CopyRandomAccessIterator& operator+=(typename Container::difference_type n) noexcept {
            (n >= 0 ? _pos += size_t(n) : _pos -= size_t(-n));
            return *this;
        }
        CopyRandomAccessIterator operator+(typename Container::difference_type n) const noexcept {
            CopyRandomAccessIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        CopyRandomAccessIterator& operator--() noexcept { --_pos; return *this; }
        CopyRandomAccessIterator operator--(int) noexcept {
            CopyRandomAccessIterator tmp(_pos);
            operator--();
            return tmp;
        }
        CopyRandomAccessIterator& operator-=(typename Container::difference_type n) noexcept {
            (n >= 0 ? _pos -= size_t(n) : _pos += size_t(-n));
            return *this;
        }
        CopyRandomAccessIterator operator-(typename Container::difference_type n) const noexcept {
            CopyRandomAccessIterator tmp(*this);
            tmp -= n;
            return tmp;
        }
        typename Container::difference_type operator-(const CopyRandomAccessIterator& rhs) const noexcept {
            return typename Container::difference_type(_pos >= rhs._pos ? _pos - rhs._pos : -(rhs._pos - _pos));
        }

        /*!
         Swap with another iterator.
         */
        void swap(CopyRandomAccessIterator& b) noexcept {
            if (this != &b) {
                std::swap(_container, b._container);
                std::swap(_pos, b._pos);
            }
        }

    private:
        const Container*    _container = nullptr;
        size_t              _pos = 0;
    };

    template <typename Container>
    inline CopyRandomAccessIterator<Container> operator+(typename Container::difference_type n,
                                                         const CopyRandomAccessIterator<Container>& c) noexcept
    {
        return (c + n);
    }

}}}


/*!
 Swap two iterators.
 Note this is added to the std namespace so that the stl code will pick it up when necessary.

 @throws std::invalid_argument if the two iterators are not from the same container.
 */
namespace std {
    template <typename Container, typename T>
    inline void swap(kss::util::iterators::ForwardIterator<Container, T>& a,
                     kss::util::iterators::ForwardIterator<Container, T>& b)
    {
        a.swap(b);
    }

    template <typename Container>
    inline void swap(kss::util::iterators::RandomAccessIterator<Container>& a,
                     kss::util::iterators::RandomAccessIterator<Container>& b)
    {
        a.swap(b);
    }

    template <typename Container>
    inline void swap(kss::util::iterators::ConstRandomAccessIterator<Container>& a,
                     kss::util::iterators::ConstRandomAccessIterator<Container>& b)
    {
        a.swap(b);
    }

    template <typename Container>
    inline void swap(kss::util::iterators::CopyRandomAccessIterator<Container>& a,
                     kss::util::iterators::CopyRandomAccessIterator<Container>& b)
    {
        a.swap(b);
    }
}

#endif
