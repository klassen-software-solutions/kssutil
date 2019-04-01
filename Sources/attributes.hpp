//
//  attributes.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2018-06-15.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

/*!
 \file
 \brief General attributes API.
 */

#ifndef kssutil_attributes_hpp
#define kssutil_attributes_hpp

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "convert.hpp"

namespace kss { namespace util {

    /*!
     \brief Adds an attribures API to subclasses.

     Base class used to add an attributes API to other classes. Simply subclass from
     this one in order to add the API.
     */
    class Attributes {
    public:
        using attribute_map_t = std::map<std::string, std::string>;

        virtual ~Attributes() = default;

        /*!
         Add/replace an attribute.
         @throws std::invalid_argument if the key is empty (An empty value is acceptable.)
         @throws any exception that adding to an std::map may throw.
         */
        void setAttribute(const std::string& key, const std::string& value);

        /*!
         Obtain an attribute and convert to the type T.
         @throws std::invalid_argument if the key is not currently in the attribute map.
         @throws std::system_error if the key exists but the value cannot be converted to type T.
         */
        template <class T>
        T attribute(const std::string& key) const {
            const auto ra = rawAttribute(key);
            return strings::convert<T>(ra);
        }

        /*!
         Obtain an attribute, or a default value if it does not exist, and convert it
         to the type T.
         @throws any exception thrown by the attribute() method
         */
        template <class T>
        inline T attributeWithDefault(const std::string& key,
                                      const T& defaultValue = T()) const
        {
            return (hasAttribute(key) ? attribute<T>(key) : defaultValue);
        }

        /*!
         Returns true if an attribute of the given key exists, and false otherwise.
         @throws std::invalid_argument if key is empty
         */
        bool hasAttribute(const std::string& key) const;

        /*!
         Read-only access to the attribute map.
         */
        inline const attribute_map_t& attributes() const noexcept {
            return _attributes;
        }

        /*!
         Returns a vector of all the attribute keys.
         @throws any exception that creating a vector may throw
         */
        std::vector<std::string> attributeKeys() const;

    protected:
        Attributes() = default;
        Attributes(const Attributes&) = default;
        Attributes(Attributes&&) = default;
        Attributes& operator=(const Attributes&) = default;
        Attributes& operator=(Attributes&&) = default;

        /*!
         Raw access to an attribute string.
         @throws std::invalid_argument if the key is not currently in the attribute map.
         */
        std::string rawAttribute(const std::string& key) const;

        /*!
         Write access to the attribute map.
         */
        attribute_map_t& attributes() noexcept { return _attributes; }

    private:
        attribute_map_t _attributes;
    };
}}

#endif
