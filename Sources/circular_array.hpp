// \file       circular_array.hpp
// \author     Steven W. Klassen (klassens@acm.org)
// \date       Sat Sep 20 2003
// \brief      Circular array container.
//
// This file is Copyright (c) 2003 by Klassen Software Solutions. All rights reserved.
// Licensing follows the MIT License.
//

#ifndef kssutil_circular_array_hpp
#define kssutil_circular_array_hpp

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>

#include <kss/util/algorithm.hpp>
#include <kss/util/iterator.hpp>
#include <kss/util/memory.hpp>

/*!
 \file
 \brief Implementation of a circular array.
 */

namespace kss { namespace util { namespace containers {

    /*!
     \brief Almost contiguous array referenced in a circular manner.

     A circular array is a container similar to a vector in that it
     contains randomly accessible, mostly contiguous, elements.  It
     differs in that elements may be pushed and popped at either the
     start or the end, but no interior insertions are allowed.  In
     addition it will not reallocate memory except as a result of a
     resize() call.  The intention is that this should be used for
     situations where you need to change the endpoints (not allowed
     in a vector but allowed in a deque or a list) but you do not
     want per-operation memory allocations (like a vector but unlike
     a deque or a list).  It is particularly useful for stack and
     queue implementations with a known maximum size.

     The circular array is implemented as an almost contiguous array that may
     be referenced in a circular manner.  In particular, going past
     the end of the array puts you back at the beginning.  The logical
     start and end points are recorded internally and modified as
     elements are pushed and popped.  The random access methods such
     as the [] operator and the at() method account for this and
     are always specified in relation to the logical start.  Hence,
     for example, at(0) will always give you the first element in
     the circular array (as expected), but the actual address of this
     will change as items are pushed and popped.

     The method and operator efficiencies are described below:

     - Element Access (front,back,[],at) - O(1)
     - Stack and Queue (push_back,pop_back,push_front,pop_front) - O(1)
     - List Operations (clear) - O(n)
     - Iterators - O(1)
     */
    template <class T, class A = std::allocator<T>>
    class CircularArray : public kss::util::AddRelOps<CircularArray<T, A>> {
    public:
        using value_type = T;
        using allocator_type = A;
        using size_type = typename A::size_type;
        using difference_type = typename A::difference_type;
        using iterator = kss::util::iterators::RandomAccessIterator<CircularArray>;
        using const_iterator = kss::util::iterators::ConstRandomAccessIterator<CircularArray>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using pointer = typename A::pointer;
        using const_pointer = typename A::const_pointer;
        using reference = typename A::reference;
        using const_reference = typename A::const_reference;

        /*!
         Construct a circular array. Note that the InputIterator version may reallocate
         memory many times if cap is not large enough to hold all the elements. (Since
         we cannot determine the number of input iterators beforehand.)
         */
        explicit CircularArray(size_type cap = size_type(10), const A& allocator = A())
        : _allocator(allocator)
        {
            init(cap);

            // postconditions
            assert(_array != nullptr);
            assert(_capacity == cap);
            assert(_size == 0);
            assert(_first == 0);
            assert(_last == _size);
        }

        CircularArray(size_type n,
                      const value_type& val,
                      size_type cap = size_type(10),
                      const A& allocator = A())
        : _allocator(allocator)
        {
            init(std::max(n, cap));
            assign(n, val);

            // postconditions
            assert(_array != nullptr);
            assert(_capacity == std::max(n, cap));
            assert(_size == n);
            assert(_first == 0);
            assert((_last == _size) || (_last == 0 && _size == _capacity));
        }

        template <class InputIterator>
        CircularArray(InputIterator first,
                      InputIterator last,
                      size_type cap = size_type(10),
                      const A& allocator = A())
        : _allocator(allocator)
        {
            init(cap);
            assign(first, last);

            // postconditions
            assert(_array != nullptr);
            assert(_capacity >= cap);
            assert(_capacity >= _size);
            assert(_first == 0);
            assert((_last == _size) || (_last == 0 && _size == _capacity));
        }

        CircularArray(const CircularArray& ca,
                      size_type cap = size_type(10),
                      const A& allocator = A())
        : _allocator(allocator)
        {
            init(std::max(ca.size(), cap));
            assign(ca.begin(), ca.end());

            // postconditions
            assert(_array != nullptr);
            assert(_capacity == std::max(ca.size(), cap));
            assert(_size == ca.size());
            assert(_first == 0);
            assert((_last == _size) || (_last == 0 && _size == _capacity));
            assert(*this == ca);
        }

        CircularArray(CircularArray&& ca) {
            _array = ca._array;
            _capacity = ca._capacity;
            _first = ca._first;
            _last = ca._last;
            _size = ca._size;
            _allocator = ca._allocator;

            ca._array = nullptr;
            ca._capacity = ca._first = ca._last = ca._size = 0;

            // postconditions
            assert(_array != nullptr);
            assert(_capacity >= _size);
            assert(_capacity > 0);
            assert(ca._array == nullptr);
            assert(ca._size == 0);
            assert(ca._capacity == 0);
        }

        CircularArray(std::initializer_list<value_type> il,
                      size_type cap = size_type(10),
                      const A& allocator = A())
        : _allocator(allocator)
        {
            init(std::max(il.size(), cap));
            assign(il);

            // postconditions
            assert(_array != nullptr);
            assert(_capacity == std::max(il.size(), cap));
            assert(_size == il.size());
            assert(_first == 0);
            assert((_last == _size) || (_last == 0 && _size == _capacity));
        }

        ~CircularArray() noexcept {
            teardown();
        }

        /*!
         Assignment. These will affect both the capacity and the contents.
         @throws std::bad_alloc if there is not enough memory.
         @throws any exceptions that value_type constructor/destructor may throw
         */
        CircularArray& operator=(const CircularArray& ca) {
            if (&ca != this) {
                teardown();
                init(ca.capacity());
                for (const value_type& v : ca) {
                    push_back(v);
                }
            }

            // postconditions
            assert(_array != nullptr);
            assert(_capacity >= ca._capacity);
            assert(*this == ca);
            return *this;
        }

        CircularArray& operator=(CircularArray&& ca) {
            if (&ca != this) {
                teardown();
                _array = ca._array;
                _capacity = ca._capacity;
                _first = ca._first;
                _last = ca._last;
                _size = ca._size;
                _allocator = ca._allocator;
                ca.teardown();
            }

            // postconditions
            assert(_array != nullptr);
            assert(_capacity >= _size);
            assert(_capacity > 0);
            assert(ca._array == nullptr);
            assert(ca._capacity == 0);
            assert(ca._size == 0);
            return *this;
        }

        template <class InputIterator>
        void assign(InputIterator first, InputIterator last) {
            clear();
            for (InputIterator it = first; it != last; ++it) {
                ensure_room_for_one_more();
                push_back(*it);
            }

            // postconditions
            assert(_array != nullptr);
            assert(_capacity > 0);
            assert(_capacity >= _size);
        }

        void assign(size_type n, const value_type& val) {
            clear();
            reserve(n);
            for (size_type i = 0; i < n; ++i) {
                push_back(val);
            }

            // postconditions
            assert(_array != nullptr);
            assert(_capacity >= _size);
            assert(_size == n);
        }

        void assign(std::initializer_list<value_type> il) {
            clear();
            reserve(il.size());
            assign(il.begin(), il.end());

            // postconditions
            assert(_array != nullptr);
            assert(_capacity >= _size);
            assert(_size == il.size());
        }

        /*!
         Iterators. Note that these satisfy the requirements of a random access iterator.
         */
        iterator begin()                                { return iterator(*this); }
        iterator end()                                  { return iterator(*this, true); }
        const_iterator begin() const                    { return const_iterator(*this); }
        const_iterator end() const                      { return const_iterator(*this, true); }
        const_iterator cbegin() const                   { return const_iterator(*this); }
        const_iterator cend() const                     { return const_iterator(*this, true); }
        reverse_iterator rbegin() noexcept              { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept                { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const noexcept  { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const noexcept    { return const_reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crend() const noexcept   { return const_reverse_iterator(begin()); }

        /*!
         Capacity. Note that resize will grow the capacity if needed, but will never
         shrink the capacity. Similarly reserve will grow the allowable space, but
         will never shrink it. The only way to shrink the actual memory used is to
         call shrink_to_fit() which will temporarily allocate more memory before
         completing the shrink. (i.e. It does not shrink "in place.")
         */
        size_type size() const noexcept     { return _size; }
        size_type max_size() const noexcept { return _allocator.max_size(); }
        size_type capacity() const noexcept { return _capacity; }
        bool empty() const noexcept         { return (_size == 0); }

        void resize(size_type n, const value_type& val = value_type()) {
            while (size() > n) {
                pop_back();
            }
            if (size() < n) {
                reserve(n);
                do { push_back(val); } while (size() < n);
            }

            // postconditions
            assert(_array != nullptr);
            assert(_size == n);
            assert(_capacity >= n);
        }

        void reserve(size_type cap) {
            if (cap <= _capacity) {
                return;
            }

            CircularArray ca(*this, cap, _allocator);
            swap(ca);

            // postconditions
            assert(_array != nullptr);
            assert(_capacity >= cap);
            assert(_capacity >= _size);
        }

        void shrink_to_fit() {
            if (capacity() > size()) {
                CircularArray tmp(*this, size(), _allocator);
                swap(tmp);
            }

            // postconditions
            assert(_capacity == _size);
        }

        /*!
         Accessors. Note that operator[] provides unchecked access while at
         provides checked access and will throw an exception if necessary. Also note
         that front() and back() are unchecked access in this implementation.
         */
        reference operator[](size_type n) noexcept {
            // preconditions
            assert(_array != nullptr);
            assert(_size > n);

            size_type pos = _first + n;
            if (pos >= _capacity) {
                pos -= _capacity;
            }

            // postconditions
            assert((pos >= _first && pos < _capacity)
                   || (pos < _last && _first > _last)
                   || (pos >= _first && pos < _last));
            return _array[pos];
        }

        const_reference operator[](size_type n) const noexcept {
            // preconditions
            assert(_array != nullptr);
            assert(_size > n);

            size_type pos = _first + n;
            if (pos >= _capacity) {
                pos -= _capacity;
            }

            // postconditions
            assert((pos >= _first && pos < _capacity)
                   || (pos < _last && _first > _last)
                   || (pos >= _first && pos < _last));
            return _array[pos];
        }

        reference at(size_type n) {
            // preconditions
            assert(_array != nullptr);

            if (n >= _size) {
                throw std::out_of_range("n is out of range of this circular_array");
            }
            size_type pos = _first + n;
            if (pos >= _capacity) {
                pos -= _capacity;
            }

            // postconditions
            assert((pos >= _first && pos < _capacity)
                   || (pos < _last && _first > _last)
                   || (pos >= _first && pos < _last));
            return _array[pos];
        }

        const_reference at(size_type n) const {
            // preconditions
            assert(_array != nullptr);

            if (n >= _size) {
                throw std::out_of_range("n is out of range of this circular_array");
            }
            size_type pos = _first + n;
            if (pos >= _capacity) {
                pos -= _capacity;
            }

            // postconditions
            assert((pos >= _first && pos < _capacity)
                   || (pos < _last && _first > _last)
                   || (pos >= _first && pos < _last));
            return _array[pos];
        }

        reference front() noexcept {
            // precondijtions
            assert(_array != nullptr);
            assert(_size > 0);

            return _array[_first];
        }

        const_reference front() const noexcept {
            // precondijtions
            assert(_array != nullptr);
            assert(_size > 0);

            return _array[_first];
        }

        reference back() noexcept {
            // preconditions
            assert(_array != nullptr);
            assert(_size > 0);

            if (_last == 0) {
                return _array[_capacity-1];
            }
            else {
                return _array[_last - 1];
            }
        }

        const_reference back() const noexcept {
            // preconditions
            assert(_array != nullptr);
            assert(_size > 0);

            if (_last == 0) {
                return _array[_capacity-1];
            }
            else {
                return _array[_last - 1];
            }
        }

        /*!
         Modifiers. these will not increase the capacity. Attempting to add an item
         when we have already reached capacity will throw an exception.
         */
        void push_back(const value_type& val) {
            value_type v(val);
            push_back(std::move(v));

            // postconditions
            assert(back() == val);
        }

        void push_back(value_type&& val) {
            check_room_for_one_more();
            _allocator.construct(_array+_last, val);
            ++_last;
            ++_size;
            if (_last >= _capacity) {
                _last = 0;
            }

            // postconditions
            assert(_array != nullptr);
            assert(_size > 0);
            assert(_capacity >= _size);
        }

        void pop_back() noexcept {
            // preconditions
            assert(_array != nullptr);
            assert(_size > 0);

            _last = (_last == 0 ? _capacity - 1 : _last - 1);
            --_size;
            _allocator.destroy(_array+_last);

            // postconditions
            assert(_array != nullptr);
            assert(_capacity > 0);
        }

        void push_front(const value_type& val) {
            value_type v(val);
            push_front(std::move(v));

            // postconditions
            assert(front() == val);
        }

        void push_front(value_type&& val) {
            check_room_for_one_more();
            _first = (_first == 0 ? _capacity - 1 : _first - 1);
            ++_size;
            _allocator.construct(_array+_first, val);

            // postconditions
            assert(_array != nullptr);
            assert(_size > 0);
            assert(_capacity >= _size);
        }

        void pop_front() noexcept {
            // preconditions
            assert(_array != nullptr);
            assert(_size > 0);

            _allocator.destroy(_array+_first);
            ++_first;
            --_size;
            if (_first >= _capacity) {
                _first = 0;
            }

            // postconditions
            assert(_array != nullptr);
            assert(_capacity > 0);
        }

        void swap(CircularArray& ca) noexcept {
            if (&ca != this) {
                std::swap(_array, ca._array);
                std::swap(_capacity, ca._capacity);
                std::swap(_first, ca._first);
                std::swap(_last, ca._last);
                std::swap(_size, ca._size);
                std::swap(_allocator, ca._allocator);
            }
        }

        void clear() noexcept {
            using kss::util::memory::destroy;
            if (_first < _last) {
                destroy(_array+_first, _array+_last, _allocator);
            }
            else if (_last < _first) {
                destroy(_array+_first, _array+_capacity, _allocator);
                destroy(_array, _array+_last, _allocator);
            }
            _first = _last = _size = 0;

            // postconditions
            assert(_size == 0);
            assert(_first == 0);
            assert(_last == 0);
        }

        /*!
         Return a copy of this container's allocator.
         */
        allocator_type get_allocator() const noexcept { return _allocator; }

        /*!
         Relational operators. These perform a lexicographical_compare. The remainder
         of the operators are provided by AddRelOps
         */
        bool operator==(const CircularArray& rhs) const noexcept {
            if (&rhs == this) {
                return true;
            }
            if (_size != rhs._size) {
                return false;
            }
            return !kss::util::notEqual(begin(), end(), rhs.begin());
        }

        bool operator<(const CircularArray& rhs) const noexcept {
            if (&rhs == this) {
                return false;
            }
            return std::lexicographical_compare(begin(), end(), rhs.begin(), rhs.end());
        }

    private:
        void init(size_type cap) {
            _array = _allocator.allocate(cap);
            _capacity = cap;
            _first = _last = _size = 0;
        }

        void teardown() {
            if (_array) {
                using kss::util::memory::destroy;
                if (_first < _last) {
                    destroy(_array+_first, _array+_last, _allocator);
                }
                else if (_last < _first) {
                    destroy(_array+_first, _array+_capacity, _allocator);
                    destroy(_array, _array+_last, _allocator);
                }
                _allocator.deallocate(_array, _capacity);
                _array = nullptr;
            }
        }

        void ensure_room_for_one_more() {
            if (size() == capacity()) {
                size_type growth = std::max(size_type(10), capacity() / 4);
                reserve(capacity() + growth);
            }
            assert(size() < capacity());
        }

        void check_room_for_one_more() {
            if (size() == capacity()) {
                throw std::length_error("This circular_array is full.");
            }
            assert(size() < capacity());
        }

        pointer         _array;
        size_type       _capacity;
        size_type       _first;
        size_type       _last;      // one past last element
        size_type       _size;
        allocator_type  _allocator;
    };

    /*!
     Swap the contents of two arrays.
     */
    template <class T, class A>
    void swap(CircularArray<T,A>& x, CircularArray<T,A>& y) {
        x.swap(y);
    }
}}}


#endif
