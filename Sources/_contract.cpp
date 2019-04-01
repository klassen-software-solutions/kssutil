//
//  _contract.cpp
//  ksscontract
//
//  Created by Steven W. Klassen on 2018-12-14.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
// "Borrowed" from ksscontract

#include <cassert>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "_contract.hpp"

using namespace std;
using namespace kss::util::_private::contract;

using _private::Expression;

namespace {
    inline const char* localBasename(const string& path) noexcept {
        const size_t pos = path.find_last_of('/');
        return (pos == string::npos ? path.c_str() : path.c_str() + pos + 1);
    }
}

void _private::performThrowingCheck(const char *conditionType, const Expression& exp) {
    if (!exp.result) {
        assert(exp.expr != nullptr);
        assert(exp.functionName != nullptr);
        assert(exp.fileName != nullptr);

        throw invalid_argument(string(conditionType)
                               + " failed: '" + exp.expr
                               + "' in " + exp.functionName
                               + ", file " + localBasename(exp.fileName)
                               + ", line " + to_string(exp.lineNo));
    }
}

void _private::performTerminatingCheck(const char *conditionType, const Expression& exp) {
    if (!exp.result) {
        assert(exp.expr != nullptr);
        assert(exp.functionName != nullptr);
        assert(exp.fileName != nullptr);

        cerr << conditionType << " failed: '" << exp.expr << "'" << endl;
        cerr << "   in " << exp.functionName << endl;
        cerr << "   file: " << exp.fileName << ", line: " << exp.lineNo << endl;
        terminate();
    }
}
