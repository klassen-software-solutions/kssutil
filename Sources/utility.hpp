//
//  utility.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2019-01-17.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssutil_utility_hpp
#define kssutil_utility_hpp

#include <exception>
#include <iostream>

namespace kss { namespace util {

    namespace _private {
        // Use these macros if a condition fails in a header file. (In a cpp file
        // use the contract API.)
#       define _KSSUTIL_PRECONDITIONS_FAILED { std::cerr << "preconditions failed " << __FILE__ << ", " << __LINE__ << std::endl; std::terminate(); }
#       define _KSSUTIL_POSTCONDITIONS_FAILED { std::cerr << "postconditions failed " << __FILE__ << ", " << __LINE__ << std::endl; std::terminate(); }
    }
}}

#endif
