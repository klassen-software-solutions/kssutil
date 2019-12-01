//
//  tokenizer.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2012-12-22.
//  Copyright (c) 2012 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/contract/all.h>

#include "tokenizer.hpp"

using namespace std;
using namespace kss::util::strings;

namespace contract = kss::contract;


Tokenizer::Tokenizer(const string& s,
                     const string& delim,
                     string::size_type start,
                     string::size_type end)
: _s(s), _delim(delim)
{
    contract::parameters({
        KSS_EXPR(!delim.empty())
    });

    _lastPos = start;
    _end = min(end, _s.length());
    if (_lastPos == _end) {
        _lastPos = _end+1;
    }

    contract::postconditions({
        KSS_EXPR(!_delim.empty()),
        KSS_EXPR((_lastPos == start) || (_lastPos == (_end+1))),
        KSS_EXPR(_end <= end)
    });
}

Tokenizer::Tokenizer(string&& s,
                     const string& delim,
                     string::size_type start, string::size_type end)
: _s(s), _delim(delim)
{
    contract::parameters({
        KSS_EXPR(!delim.empty())
    });

    _lastPos = start;
    _end = min(end, _s.length());
    if (_lastPos == _end) {
        _lastPos = _end+1;
    }

    contract::postconditions({
        KSS_EXPR(!_delim.empty()),
        KSS_EXPR((_lastPos == start) || (_lastPos == (_end+1))),
        KSS_EXPR(_end <= end)
    });
}


// Obtain the next token.
string& Tokenizer::next(string& token) {
    contract::preconditions({
        KSS_EXPR(hasAnother() == true)
    });

    string::size_type pos = _s.find_first_of(_delim, _lastPos);

    if (pos >= _end) {
        // final token
        if (_lastPos < _end) {
            token = _s.substr(_lastPos, (_end - _lastPos));
        }
        else {
            // Final token is the empty token.
            token = string();
        }
        _lastPos = _end+1;
    }
    else if (pos == _lastPos) {
        // empty token
        ++_lastPos;
        token = string();
    }
    else {
        // next token
        token = _s.substr(_lastPos, (pos - _lastPos));
        _lastPos = pos + 1;
    }

    return token;
}
