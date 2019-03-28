//
//  uuid.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-08-28.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//
//
// 	Permission is hereby granted to use, modify, and publish this file without restriction other
// 	than to recognize that others are allowed to do the same.
//


#include "_contract.hpp"
#include "uuid.hpp"

using namespace std;
using namespace kss::util;
namespace contract = kss::util::_private::contract;


// MARK: Compatibility Items

#if defined(__linux)
    typedef char uuid_string_t[37];
#endif


// MARK: UUID IMPLEMENTATION

UUID::UUID() {
    uuid_clear(_uid);

    contract::postconditions({
        KSS_EXPR(bool(*this) == false)
    });
}


UUID::UUID(const UUID& uid) {
    uuid_copy(_uid, uid._uid);

    contract::postconditions({
        KSS_EXPR(*this == uid)
    });
}


UUID::UUID(uuid_t uid) {
    uuid_copy(_uid, uid);

    contract::postconditions({
        KSS_EXPR(uuid_compare(_uid, uid) == 0)
    });
}


UUID::UUID(const string& suid) {
    if (uuid_parse(suid.c_str(), _uid) == -1) {
        const string msg = string("could not parse '") + suid.c_str() + "' as a uuid";
        throw invalid_argument(msg);
    }

    contract::postconditions({
        KSS_EXPR(bool(*this) == true)
    });
}


UUID::operator string() const noexcept {
    if (bool(*this) == false) {
        return "";
    }

    uuid_string_t suid;
    uuid_unparse_lower(_uid, suid);
    return string(suid);
}


void UUID::copyInto(uuid_t* uid) const {
    contract::parameters({
        KSS_EXPR(uid != nullptr)
    });

    uuid_copy(*uid, _uid);

    contract::postconditions({
        KSS_EXPR(uuid_compare(_uid, *uid) == 0)
    });
}


UUID& UUID::operator=(const UUID& uid) noexcept {
    if (this != &uid) {
        uuid_copy(_uid, uid._uid);
    }

    contract::postconditions({
        KSS_EXPR(*this == uid)
    });
    return *this;
}


UUID& UUID::operator=(uuid_t uid) noexcept {
    uuid_copy(_uid, uid);

    contract::postconditions({
        KSS_EXPR(uuid_compare(_uid, uid) == 0)
    });
    return *this;
}


void UUID::clear() noexcept {
    uuid_clear(_uid);

    contract::postconditions({
        KSS_EXPR(bool(*this) == false)
    });
}


UUID UUID::generate() noexcept {
    UUID u;
    uuid_generate(u._uid);
    contract::postconditions({
        KSS_EXPR(bool(u) == true)
    });
    return u;
}


UUID UUID::null() noexcept {
    UUID uid;

    contract::postconditions({
        KSS_EXPR(bool(uid) == false)
    });
    return uid;
}
