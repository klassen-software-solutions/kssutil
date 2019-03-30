//
//  containerutil.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-05-18.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssutil_containerutil_hpp
#define kssutil_containerutil_hpp

#include <algorithm>
#include <functional>

namespace kss { namespace util { namespace containers {

    /*!
     contains returns true if the container contains the given element as determined
     by using the containers find method.
     @throws any exception that Container::find() may throw
     */
    template <class Container>
    inline bool contains(const Container& c, const typename Container::key_type& a) {
        return (c.find(a) != c.end());
    }

    /*!
     hasAtLeast returns true if the given container has reached at
     least a specified size.  It will work on any container that supports
     the size() method.
     */
    template <class Container>
    inline bool hasAtLeast(const Container& c, typename Container::size_type sz) noexcept {
        return (c.size() >= sz);
    }


    /*!
     This predicate returns true if the container is full. That is, if it's
     size equals its current capacity. This will work on any container that
     supports both the size() and capacity() methods.
     */
    template <class Container>
    inline bool isFull(const Container& c) noexcept {
        return (c.size() == c.capacity());
    }

    /*!
     Erase items from a container provided that they match the given predicate. This
     is a convenience wrapper around a remove_if call and the container's erase call.
     */
    template <class Container, class UnaryPredicate>
    void eraseIf(Container& c, UnaryPredicate pred) {
        c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
    }

    /*!
     Returns an iterator to the item in the container that matches the query, or c.end()
     if it does not exist. This is performed by calling std::find_if and passing in
     c.begin() and c.end(). So it should work on any container that supports the
     InputIterator semantics.
     @throws any exception that pred throws
     */
    template <class Container, class UnaryPredicate>
    typename Container::iterator findIf(Container& c, UnaryPredicate pred) {
        return std::find_if(c.begin(), c.end(), pred);
    }

    template <class Container, class UnaryPredicate>
    typename Container::const_iterator findIf(const Container& c, UnaryPredicate pred) {
        return std::find_if(c.begin(), c.end(), pred);
    }

    /*!
     Returns true if the container contains at least one item that matches pred
     and false otherwise.
     @throws any exception that pred throws
     */
    template <class Container, class UnaryPredicate>
    inline bool containsIf(const Container& c, UnaryPredicate pred) {
        return (kss::util::containers::findIf(c, pred) != c.end());
    }

    /*!
     Applies an operation to each element of a vector-like container, and assigns the
     resulting value back into the container. This is similar to std::for_each and
     std::valarray::apply, but the index of each value is also passed to the lambda,
     allowing position based algorithms to be used.
     */
    template <class Vector>
    void apply(Vector& vec,
               const std::function<typename Vector::value_type(size_t, const typename Vector::value_type&)>& fn)
    {
        const size_t len = vec.size();
        for (size_t i(0); i < len; ++i) {
            vec[i] = fn(i, vec[i]);
        }
    }
}}}

#endif
