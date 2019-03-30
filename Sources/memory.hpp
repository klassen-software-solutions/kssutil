//
//  memory.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-11-23.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssutil_memory_hpp
#define kssutil_memory_hpp

namespace kss { namespace util { namespace memory {

    /*!
     This is similar to std::default_delete except that this class actually does nothing.
     It's purpose is for areas where you want the semantics of a shared_ptr but do not
     actually want the pointer deleted when it is done because it is being managed
     by some other means.
     */
    template <class T>
    class NullDelete {
    public:
        void operator()(T* ptr) const noexcept {}
    };

    /*!
     Destroy the elements from [start,finish).
     */
    template <class Iterator, class Allocator>
    void destroy(Iterator start, Iterator finish, Allocator& allocator) {
        while (start != finish) {
            allocator.destroy(start++);
        }
    }

    /*!
     Destroy n elements starting at start.
     */
    template <class Iterator, class Allocator>
    void destroyN(Iterator start, typename Iterator::size_type n, Allocator& allocator) {
        for (typename Iterator::size_type i = 0; i < n; ++i) {
            allocator.destroy(start++);
        }
    }

}}}

#endif
