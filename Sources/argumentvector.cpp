//
//  argumentvector.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2019-03-30.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cstring>

#include <kss/contract/all.h>

#include "argumentvector.hpp"

using namespace std;
using namespace kss::util::po;
namespace contract = kss::contract;


ArgumentVector::ArgumentVector(initializer_list<string> args) {
    if (args.size() > 0) {
        add(args);
    }

    contract::postconditions({
        KSS_EXPR(argumentStrings.size() == args.size()),
        KSS_EXPR(argumentPointers.size() == argumentStrings.size()),
    });
}

ArgumentVector::ArgumentVector(const ArgumentVector& av) {
    argumentStrings = av.argumentStrings;
    rebuild();

    contract::postconditions({
        KSS_EXPR(argumentStrings == av.argumentStrings),
        KSS_EXPR(argumentPointers.size() == argumentStrings.size()),
    });
}

ArgumentVector& ArgumentVector::operator=(const ArgumentVector &av) {
    if (this != &av) {
        argumentStrings = av.argumentStrings;
        rebuild();
    }

    contract::postconditions({
        KSS_EXPR(argumentStrings == av.argumentStrings),
        KSS_EXPR(argumentPointers.size() == argumentStrings.size()),
    });
    return *this;
}

void ArgumentVector::add(const std::string &arg) {
    string s(arg);
    add(move(s));
}

void ArgumentVector::add(string&& arg) {
    contract::preconditions({
        KSS_EXPR(argumentPointers.size() == argumentStrings.size())
    });

    const auto sz = argumentStrings.size();
    argumentStrings.push_back(move(arg));
    rebuild();

    contract::postconditions({
        KSS_EXPR(argumentStrings.size() == (sz+1)),
        KSS_EXPR(argumentPointers.size() == argumentStrings.size()),
        KSS_EXPR(strcmp(argumentStrings.back().c_str(), argumentPointers.back()) == 0)
    });
}

void ArgumentVector::add(std::initializer_list<std::string> args) {
    contract::parameters({
        KSS_EXPR(args.size() > 0)
    });

    add(args.begin(), args.end());
}

int ArgumentVector::argc() const noexcept {
    contract::preconditions({
        KSS_EXPR(argumentPointers.size() == argumentStrings.size())
    });

    return static_cast<int>(argumentPointers.size());
}

char* const* ArgumentVector::argv() noexcept {
    contract::preconditions({
        KSS_EXPR(argumentPointers.size() == argumentStrings.size())
    });

    const auto sz = argumentPointers.size();
    return (sz == 0 ? nullptr : argumentPointers.data());
}

const char* const* ArgumentVector::argv() const noexcept {
    contract::preconditions({
        KSS_EXPR(argumentPointers.size() == argumentStrings.size())
    });

    const auto sz = argumentPointers.size();
    return (sz == 0 ? nullptr : argumentPointers.data());
}

void ArgumentVector::rebuild() {
    argumentPointers.clear();
    argumentPointers.reserve(argumentStrings.size());
    for (const auto& s : argumentStrings) {
        argumentPointers.push_back((char*)s.c_str());
    }
}
