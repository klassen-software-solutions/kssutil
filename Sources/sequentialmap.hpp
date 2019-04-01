//
//  sequentialmap.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-04-04.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

/*!
 \file
 \brief Map implementation that maintains the order of insertion.
 */

#ifndef kssutil_sequentialmap_hpp
#define kssutil_sequentialmap_hpp

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kss { namespace util { namespace containers {

    /*!
      \brief Combination of a list and a map

      This class combines a list and a map to provide a container where items can
      be looked up quickly (via the map) but where the iterators preserve the order
      with which things are added.

      Note that while lookups are efficient, and iterating is efficient, the removal
      of items is O(n) as it combines the O(log n) of the map with the O(n) of the
      list.

      We follow the std::map API as much as is reasonably possible.
     */
    template <class Key, class T, class Compare = std::less<Key>,
        class Alloc = std::allocator<std::pair<Key, T> >,
        class MAlloc = std::allocator<std::pair<const Key, size_t> > >
    class SequentialMap {
    public:

        // MARK: The standard type definitions.
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<Key, T>;
        using key_compare = Compare;
        using allocator_type = Alloc;
        using m_allocator_type = MAlloc;
        using reference = typename allocator_type::reference;
        using const_reference = typename allocator_type::const_reference;
        using pointer = typename allocator_type::pointer;
        using const_pointer = typename allocator_type::const_pointer;
        using iterator = typename std::vector<value_type, Alloc>::iterator;
        using const_iterator = typename std::vector<value_type, Alloc>::const_iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using difference_type = typename std::iterator_traits<iterator>::difference_type;
        using size_type = size_t;

        class value_compare;


        // MARK: Constructors
        explicit SequentialMap(const key_compare& comp = key_compare(),
                               const allocator_type& talloc = allocator_type(),
                               const m_allocator_type& malloc = m_allocator_type())
        {
            // postconditions
            assert(_vec.empty());
            assert(_map.empty());
        }

        template <class InputIterator>
        SequentialMap(InputIterator first, InputIterator last,
                      const key_compare& comp = key_compare(),
                      const allocator_type& alloc = allocator_type())
        {
            insert(first, last);

            // postconditions
            assert(_vec.size() == _map.size());
        }

        SequentialMap(const SequentialMap&) = default;
        SequentialMap(SequentialMap&&) = default;

        ~SequentialMap() = default;

        SequentialMap& operator=(const SequentialMap& x) = default;
        SequentialMap& operator=(SequentialMap&&) = default;

        // MARK: Iterators
        iterator                begin() noexcept         { return _vec.begin(); }
        const_iterator          begin() const noexcept   { return _vec.begin(); }
        iterator                end() noexcept           { return _vec.end(); }
        const_iterator          end() const noexcept     { return _vec.end(); }
        reverse_iterator        rbegin() noexcept        { return _vec.rbegin(); }
        const_reverse_iterator  rbegin() const noexcept  { return _vec.rbegin(); }
        reverse_iterator        rend() noexcept          { return _vec.rend(); }
        const_reverse_iterator  rend() const noexcept    { return _vec.rend(); }
        const_iterator          cbegin() const noexcept  { return _vec.begin(); }
        const_iterator          cend() const noexcept    { return _vec.end(); }
        const_reverse_iterator  crbegin() const noexcept { return _vec.rbegin(); }
        const_reverse_iterator  crend() const noexcept   { return _vec.rend(); }


        // MARK: Capacity
        bool        empty() const noexcept       { return _map.empty(); }
        size_type   size() const noexcept        { return _map.size(); }
        size_type   max_size() const noexcept    { return std::min(_map.max_size(), _vec.max_size()); }


        // MARK: Element access
        mapped_type& operator[](const key_type& k) {
            // preconditions
            assert(_vec.size() == _map.size());

            iterator it = find(k);
            if (it == end()) {
                std::pair<iterator, bool> p = insert(std::make_pair(k, mapped_type()));
                it = p.first;
            }

            // postconditions
            assert(_vec.size() == _map.size());
            return it->second;
        }

        mapped_type& at(const key_type& k) {
            // preconditions
            assert(_vec.size() == _map.size());

            iterator it = find(k);
            if (it == end()) {
                throw std::out_of_range("the given key is not found in the map");
            }

            // postconditions
            assert(_vec.size() == _map.size());
            return it->second;
        }

        const mapped_type& at(const key_type& k) const {
            // preconditions
            assert(_vec.size() == _map.size());

            const_iterator it = find(k);
            if (it == end()) {
                throw std::out_of_range("the given key is not found in the map");
            }

            // postconditions
            assert(_vec.size() == _map.size());
            return it->second;
        }


        // MARK: Modifiers
        std::pair<iterator, bool> insert(const value_type& val) {
            // preconditions
            assert(_vec.size() == _map.size());

            bool wasInserted = false;
            iterator it = find(val.first);
            if (it == end()) {
                it = _vec.insert(_vec.end(), val);
                _map.insert(std::make_pair(val.first, _vec.size()-1));
                wasInserted = true;
            }

            // postconditions
            assert(_vec.size() == _map.size());
            return std::make_pair(it, wasInserted);
        }

        iterator insert(iterator position, const value_type& val) {
            return insert(val).first;
        }

        template <class InputIterator>
        void insert(InputIterator first, InputIterator last) {
            // preconditions
            assert(_vec.size() == _map.size());

            for (InputIterator it = first; it != last; ++it) {
                insert(*it);
            }

            // postconditions
            assert(_vec.size() == _map.size());
        }

        // Note that invalid_argument is thrown if first and last are not in
        // the map or are not in the correct order. This differs from
        // map::erase where the behaviour is undefined.
        void erase(iterator position) {
            erase(position, position+1);
        }

        size_type erase(const key_type& k) {
            // preconditions
            assert(_vec.size() == _map.size());

            size_type numErased { 0 };
            iterator it = find(k);
            if (it != end()) {
                erase(it);
                ++numErased;
            }

            // postconditions
            assert(_vec.size() == _map.size());
            return numErased;
        }

        // Note that invalid_argument is thrown if first and last are not in
        // the map or are not in the correct order. This differs from
        // map::erase where the behaviour is undefined.
        void erase(iterator first, iterator last) {
            // preconditions
            assert(first >= begin());
            assert(last >= first);
            assert(_vec.size() == _map.size());

            difference_type i = first - begin();
            difference_type n = last - first;
            if (i < 0 || n < 0) {
                throw std::invalid_argument("iterator(s) are not valid for this SequentialMap");
            }

            // Need to remove the items from both _vec and _map, then reduce
            // the indices in _map for all items >= i by n.
            for (iterator it = first; it != last; ++it) {
                _map.erase(it->first);
            }
            _vec.erase(first, last);
            for (typename std::map<Key, size_t, Compare, MAlloc>::iterator it = _map.begin(), stop = _map.end(); it != stop; ++it) {
                if (it->second >= size_type(i)) {
                    it->second -= size_type(n);
                }
            }

            // postconditions
            assert(_vec.size() == _map.size());
        }

        void swap(SequentialMap& x) {
            _vec.swap(x._vec);
            _map.swap(x._map);

            // postconditions
            assert(_vec.size() == _map.size());
            assert(x._vec.size() == x._map.size());
        }

        void clear() noexcept {
            _vec.clear();
            _map.clear();

            // postconditions
            assert(_vec.empty());
            assert(_map.empty());
        }


        // MARK: Observers
        key_compare         key_comp() const                 { return Compare(); }
        value_compare       value_comp() const               { return value_compare(Compare()); }
        allocator_type      get_allocator() const noexcept   { _vec.get_allocator(); }
        m_allocator_type    get_m_allocator() const noexcept { _map.get_allocator(); }


        // MARK: Operations
        iterator find(const key_type& k) {
            // preconditions
            assert(_vec.size() == _map.size());

            iterator retIt = end();
            typename std::map<Key, size_t, Compare, MAlloc>::iterator it = _map.find(k);
            if (it != _map.end()) {
                size_t maxIncr = std::numeric_limits<difference_type>::max();
                size_t remain = it->second;
                retIt = _vec.begin();
                while (remain > maxIncr) {
                    retIt += difference_type(maxIncr);
                    remain -= maxIncr;
                }
                retIt += difference_type(remain);
            }

            // postconditions
            assert(_vec.size() == _map.size());
            return retIt;
        }

        const_iterator find(const key_type& k) const {
            // preconditions
            assert(_vec.size() == _map.size());

            const_iterator retIt = end();
            typename std::map<Key, size_t, Compare, MAlloc>::const_iterator it = _map.find(k);
            if (it != _map.end()) {
                size_t maxIncr = std::numeric_limits<difference_type>::max();
                size_t remain = it->second;
                retIt = _vec.begin();
                while (remain > maxIncr) {
                    retIt += difference_type(maxIncr);
                    remain -= maxIncr;
                }
                retIt += difference_type(remain);
            }

            // postconditions
            assert(_vec.size() == _map.size());
            return retIt;
        }

        size_type count(const key_type& k) const {
            assert(_vec.size() == _map.size());
            return (_map.find(k) == _map.end() ? 0 : 1);
        }

        class value_compare {
            friend class SequentialMap;
        protected:
            Compare comp;
            value_compare(Compare c) : comp(c) {}
        public:
            typedef bool result_type;
            typedef value_type first_argument_type;
            typedef value_type second_argument_type;
            bool operator() (const value_type& x, const value_type& y) const { return comp(x.first, y.first); }
        };

    private:
        std::vector<std::pair<Key, T>, Alloc>   _vec;
        std::map<Key, size_t, Compare, MAlloc>  _map;
    };


    template <class Key, class T, class Compare, class Alloc, class MAlloc>
    inline void swap(SequentialMap<Key, T, Compare, Alloc, MAlloc>& x,
                     SequentialMap<Key, T, Compare, Alloc, MAlloc>& y)
    {
        x.swap(y);
    }

}}}

#endif
