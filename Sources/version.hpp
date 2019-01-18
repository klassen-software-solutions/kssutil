//
//  version.hpp
//  kssio
//
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssutil_version_hpp
#define kssutil_version_hpp

#include <string>

namespace kss { namespace util {

    /*!
     Returns a string of the form x.y.z<optional tags> that describes the version
     of this library.
     */
    std::string version() noexcept;

    /*!
     Returns the text of the software license.
     */
    std::string license() noexcept;

}}

#endif
