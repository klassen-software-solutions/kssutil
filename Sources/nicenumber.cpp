//
//  nicenumber.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2018-03-12.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cmath>

#include "nicenumber.hpp"


using namespace std;
using namespace kss::util;


// Find a "nice" number. This code is based on a Javascript algorithm found at
// https://stackoverflow.com/questions/361681/algorithm-for-nice-grid-line-intervals-on-a-graph.
double kss::util::niceNumber(double originalNumber) noexcept {

    // Determine the magnitude of the original number.
    static const double ln10 = log(10);
    const double mag = floor(log(originalNumber) / ln10);
    const double magPower = pow(10., mag);

    // Obtain the most significant digit, promote it to a 1, 2 or 5, then return it
    // scaled by the power.
    double msd = round(originalNumber / magPower);
    if (msd > 5.) {
        msd = 10.;
    }
    else if (msd > 2.) {
        msd = 5.;
    }
    else if (msd > 1.) {
        msd = 2.;
    }
    else {
        // no action needed
    }

    return msd * magPower;
}

