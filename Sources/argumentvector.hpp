//
//  argumentvector.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2019-03-30.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef argumentvector_hpp
#define argumentvector_hpp

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

namespace kss { namespace util { namespace po {

    /*!
     ArgumentVector is used to create argv type data. It's primary use case is in the
     generation of unit tests, but it is also useful if you need to call the various
     forms of execv.

     Basically, it allows you to create an ordered list of strings, and then treat
     them as if they were a "char *const argv[]".

     It is made part of the po (program options) namespace as it is essentially a means
     of creating program options.

     One note about the character pointers returned. Most of the APIs that required an
     argument vector require an array of `char*`, not an array of `const char*`. To
     that end we make unsafe casts when generating the array in order to fulfill that
     requirement. This should be ok for the intended usages in that it is no less
     safe than changes made to the argv of a `main` program. The key is that while the
     OS is allowed to make changes to the argument vector, those changes are not allowed
     to grow the space of any string.
     */
    class ArgumentVector {
    public:

        /*!
         Construct the initial vector. Note that the first argument should be the
         program name, but that is not enforced.
         */
        explicit ArgumentVector(std::initializer_list<std::string> args = std::initializer_list<std::string>());
        ArgumentVector(const ArgumentVector& av);
        ArgumentVector(ArgumentVector&&) = default;
        ArgumentVector& operator=(const ArgumentVector& av);
        ArgumentVector& operator=(ArgumentVector&&) = default;
        ~ArgumentVector() = default;

        /*!
         Add arguments to the vector.
         */
        void add(const std::string& arg);
        void add(std::string&& arg);
        void add(std::initializer_list<std::string> args);

        template <class InputIterator>
        void add(InputIterator first, InputIterator last) {
            if (first == last) {
                throw std::invalid_argument("first and last iterator are the same");
            }
            for (auto it = first; it != last; ++it) {
                argumentStrings.push_back(*it);
            }
            rebuild();
        }

        /*!
         Return the argument count as an int.
         */
        int argc() const noexcept;

        /*!
         Return the argument vector. Note that if there are no arguments, (i.e. if
         argc() would return 0), then nullptr is returned.
         */
        char* const* argv() noexcept;
        const char* const* argv() const noexcept;

    private:
        std::vector<std::string>    argumentStrings;
        std::vector<char*>          argumentPointers;

        void rebuild();
    };

}}}

#endif /* argumentvector_hpp */
