//
//  uuid.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-08-28.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//
// 	Permission is hereby granted to use, modify, and publish this file without restriction
// 	other than to recognize that others are allowed to do the same.
//

/*!
 \file
 \brief UUID generation and handling.
 */

#ifndef kssutil_uuid_hpp
#define kssutil_uuid_hpp


#include <stdexcept>
#include <string>
#include <uuid/uuid.h>
#include "add_rel_ops.hpp"


namespace kss { namespace util {

    /*!
      \brief C++ wrapper around the uuid_t type.

      Thin wrapper around the uuid_t type. This is done primarily to allow it to be used
      in C++ template overrides, but is useful in its own right as a more sane alternative
      to using the C API.

      Note that the methods that require parsing a uuid from a string will throw an
      invalid_argument exception if the parsing fails.
     */
    class UUID : public AddRelOps<UUID> {
    public:
        UUID();
        UUID(const UUID& uid);
        UUID(UUID&&) = default;
        explicit UUID(uuid_t uid);
        explicit UUID(const std::string& suid);

        /*!
         Returns true if the UUID is not empty and false otherwise.
         */
        explicit operator bool() const noexcept {
            return (uuid_is_null(_uid) == 0);
        }

        /*!
         Returns a string representation of the UUID. This will be the empty string
         if the UUID is empty.
         */
        explicit operator std::string() const noexcept;

        /*!
         Returns the UUID value as the C value.
         */
        const uuid_t& value() const noexcept {
            return _uid;
        }

        /*!
         Copy this object's value into the C value.
         @throws std::invalid_argument if uid is null.
         */
        void copyInto(uuid_t* uid) const;

        // Setters.
        UUID& operator=(const UUID& uid) noexcept;
        UUID& operator=(UUID&& uid) = default;
        UUID& operator=(uuid_t uid) noexcept;

        // Comparators. Note that the "missing" comparators are added via add_rel_ops.
        bool operator==(const UUID& uid) const noexcept { return (uuid_compare(_uid, uid._uid) == 0); }
        bool operator<(const UUID& uid) const noexcept  { return (uuid_compare(_uid, uid._uid) < 0); }

        /*!
         Clear a uuid (make it null).
         */
        void clear() noexcept;

        /*!
         Obtain a new, random, uuid.
         */
        static UUID generate() noexcept;

        /*!
         Obtain a new, null, uuid.
         */
        static UUID null() noexcept;

    private:
        uuid_t _uid;
    };

}}

#endif
