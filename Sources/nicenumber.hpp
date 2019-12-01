//
//  nicenumber.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2018-03-12.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

/*!
 \file
 \brief Nice number generation
 */

#ifndef kssutil_nicenumber_hpp
#define kssutil_nicenumber_hpp

namespace kss { namespace util {

    /*!
     Given a generic floating point number, return a "nice" number. The meaning of a "nice"
     number in this context is something that will look nice in a graph or a ruler.
     */
    double niceNumber(double originalNumber) noexcept;

}}

#endif
