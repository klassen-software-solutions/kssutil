//
// FILENAME:    errorchpp
// AUTHOR:      Steven Klassen
// CREATED ON:  2011-08-04
//
// DESCRIPTION: Error handling in the KSS library.
//
// This file is Copyright (c) 2011 by Klassen Software Solutions. All rights reserved.
// Licensing follows the MIT License.
//

#include <system_error>

#include "error.hpp"
#include "rtti.hpp"

using namespace std;
using namespace kss::util;

bool kss::util::tryAll(const function<void()>& fn) noexcept {
    try {
        fn();
        return true;
    }
    catch (const exception&) {
        return false;
    }
}

string kss::util::errorDescription(const exception& e) {
	if (const system_error* se = rtti::as<system_error>(e)) {
        return rtti::name(e) + ": (" + to_string(se->code().value()) + ") " + e.what();
	}
	return rtti::name(e) + ": " + e.what();
}
