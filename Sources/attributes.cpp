//
//  attributes.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2018-06-15.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/contract/all.h>

#include "attributes.hpp"

using namespace std;
using namespace kss::util;
namespace contract = kss::contract;


void Attributes::setAttribute(const string &key, const string &value) {
    contract::parameters({
        KSS_EXPR(!key.empty())
    });

    _attributes[key] = value;
}

bool Attributes::hasAttribute(const string &key) const {
    contract::parameters({
        KSS_EXPR(!key.empty())
    });

    return (_attributes.find(key) != _attributes.end());
}

vector<string> Attributes::attributeKeys() const {
    vector<string> ret;
    ret.reserve(_attributes.size());
    for (const auto& p : _attributes) {
        ret.push_back(p.first);
    }
    return ret;
}

string Attributes::rawAttribute(const string& key) const {
    const auto& it = _attributes.find(key);
    if (it == _attributes.end()) {
        throw invalid_argument("Could not find the key '" + key + "' in the attributes map.");
    }
    return it->second;
}
