// \file       algorithm.hpp
// \author     Steven W. Klassen (klassens@acm.org)
// \date       Sat May 10 2003
// \brief      Generic algorithms.
//
// This file contains a variety of generic algorithms, similar in style
// to those found in the <algorithms> header of the standard library.
//
// This file is Copyright (c) 2003 by Klassen Software Solutions. All rights reserved.
// Licensing follows the MIT License.
//


#ifndef kssutil_algorithm_hpp
#define kssutil_algorithm_hpp

#include <algorithm>
#include <utility>
#include <functional>

namespace kss { namespace util {

    /*!
     Apply an operator, filtered by a predicate, to each item.
     This method is similar to std::for_each() but adds the ability
     to specify a predicate as well as an operation. Specifically the operator
     f is applied to all items for which the predicate p(item) returns true.
     @throws any exception that Operator or Predicate may throw.
     @returns f.
     */
    template <class InputIterator, class Operator, class Predicate>
    Operator forEachIf(InputIterator first, InputIterator last, Operator f, Predicate p) {
        while (first != last) {
            if (p(*first)) f(*first);
            ++first;
        }
        return f;
    }

    /*!
     Determine if two sequences of elements differ. Similar to not_equal(first, last, first2)
     except that the binary predicate bp is used for comparison instead of
     std::equal_to.
     @throws any exceptions that bp may throw.
     @returns true if at least one inequality is found.
     */
    template <class InputIterator, class InputIterator2, class BinaryPredicate>
    bool notEqual(InputIterator first,
                  InputIterator last,
                  InputIterator2 first2,
                  BinaryPredicate bp)
    {
        std::pair<InputIterator, InputIterator2> p = std::mismatch(first, last, first2, bp);
        return (p.first != last);
    }

    /*!
     Determine if two sequences of elements differ.
     Compares two sets of iterators and returns true if at least one
     element differs. Note that ++first2 must be valid for at least
     as many calls as ++first. Also note that while InputIterator and
     InputIterator2 may be different types, they must dereference to
     compatible types.
     @throws any exceptions that the std::equal_to method may throw
     @returns true if at least one inequality is found.
     */
    template <class InputIterator, class InputIterator2>
    inline bool notEqual(InputIterator first, InputIterator last, InputIterator2 first2) {
        return notEqual(first, last, first2, std::equal_to<decltype(*first)>());
    }

}}

#endif
