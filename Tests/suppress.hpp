//
//  suppress.hpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-01-18.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#ifndef suppress_hpp
#define suppress_hpp

#include <functional>
#include <ostream>

// Temporarily suppress a stream output by capturing it and throwing it away.
// "Borrowed" from the capture() method of kssio.
void suppress(std::ostream& os, const std::function<void()>& fn);

#endif /* suppress_hpp */
