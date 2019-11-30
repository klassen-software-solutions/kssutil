//
//  programoptions.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-04-18.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

/*!
 \file
 \brief Command line argument (program option) parsing.
 */

#ifndef kssutil_programoptions_hpp
#define kssutil_programoptions_hpp

#include <initializer_list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include <kss/contract/all.h>

#include "convert.hpp"

namespace kss { namespace util { namespace po {

    /*!
     HasArgument is an enumeration which describes if a program option requires an argument.
     */
    enum class HasArgument {
        none = 0,   ///< no argument is allowed
        required,   ///< an argument is required
        optional    ///< an argument is allowed, but not required
    };

    /*!
     This constant, which is char(0), should be passed as the shortOption if no short
     form of the program option is to be allowed.
     */
    static constexpr char noShortOption = char(0);

    /*!
     This constant, which is char(1), should be passed as the shortOption if you wish
     the short option to simply be the first character of the full option name.
     */
    static constexpr char autoShortOption = char(1);

    /*!
     Description of a single program option.
     */
    struct Option {
        std::string name;                           ///< the text used as the command line argument
        std::string description;                    ///< one line description used in the help text
        char        shortOption = autoShortOption;  ///< optional short form of the option
        HasArgument hasArg = HasArgument::none;     ///< specifies if the option takes an argument
        std::string defaultValue;                   ///< default value if the option isn't present
    };

    /*!
     \brief Command line argument parsing.
     
     ProgramOptions is used to parse command lines. In addition to parsing the command
     line, it also keeps the argument state allowing to to query the results without
     having to keep track of separate variables.
     */
    class ProgramOptions {
    public:

        /*!
         Construct the program options with an optional starting list.
         */
        explicit ProgramOptions(std::initializer_list<Option> options = std::initializer_list<Option>());
        ~ProgramOptions();

        // Moving is allowed, but copying is not.
        ProgramOptions(ProgramOptions&&);
        ProgramOptions& operator=(ProgramOptions&&) noexcept;

        ProgramOptions(const ProgramOptions&) = delete;
        ProgramOptions& operator=(const ProgramOptions&) = delete;

        /*!
         Add a command line options.
         @throws std::invalid_argument if the name is invalid, or if it already exists,
            or if there is already a short option (other than none or auto) of that
            value.
         */
        void add(const Option& o);
        void add(Option&& o);
        void add(std::initializer_list<Option> options);

        template <class InputIterator>
        void add(InputIterator first, InputIterator last) {
            kss::contract::parameters({
                KSS_EXPR(first != last)
            });
            for (auto it = first; it != last; ++it) {
                add(*it);
            }
        }

        /*!
         Parse the given command line.
         @param argc the number of arguments in the vector
         @param argv the argument vector
         @param ignoreUnknownOptions if true then options that are not specified will
            be quietly ignored instead of throwing an exception.
         @throws std::invalid_argument if the command line could not be parsed given
            the options.
         */
        void parse(int argc,
                   const char * const * argv,
                   bool ignoreUnknownOptions = false);

        /*!
         Returns a string describing the usage of the command line. This will be empty
         until parse has been called successfully.
         */
        std::string usage() const;

        /*!
         Returns true if an option was specified on the command line and false
         otherwise.
         @throws std::invalid_argument if name is empty
         */
        bool hasOption(const std::string& name) const;

        /*!
         Returns the value of an option's argument, converted to the given type.

         @throws std::invalid_argument if name is empty
         @throws std::invalid_argument if there is no option of the given name.
         @throws std::system_error if the option exists but the value cannot be
            converted to the given type.
         */
        template <class T>
        T option(const std::string& name) const {
            const auto s = rawOptionValue(name);
            return kss::util::strings::convert<T>(s);
        }

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;

        std::string rawOptionValue(const std::string& name) const;
    };
}}}

#endif
