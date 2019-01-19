//
//  tokenizer.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2012-12-21.
//  Copyright (c) 2012 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssutil_tokenizer_hpp
#define kssutil_tokenizer_hpp

#include <string>
#include "iterator.hpp"

namespace kss { namespace util { namespace strings {

    /*!
     Provide a container-ish view of a string separated by tokens. This is loosly based
     on code published at http://stackoverflow.com/questions/236129/split-a-string-in-c.

     This provides either a stream based or an input iterator based view of the tokens.
     */
    class Tokenizer {
    public:
        /*!
         Container/iterator based type definitions.
         */
        using value_type = std::string;
        using iterator = kss::util::iterators::ForwardIterator<Tokenizer, std::string>;

        /*!
         Create the tokenizer for a given string and delimiter set. Note that having two
         or more delimiters in a row will lead to empty tokens (i.e. empty strings).

         @param s is the string to be split into tokens
         @param delims is the set of delimiters
         @param start is the position in s to start searching
         @param end is the position in s one after the position to end searching
         @throws std::invalid_argument if delim is an empty string
         @throws any exceptions that may be thrown by the std::string class
         */
        explicit Tokenizer(const std::string& s,
                           const std::string& delims = " \t\n\r",
                           std::string::size_type start = 0,
                           std::string::size_type end = std::string::npos);
        explicit Tokenizer(std::string&& s,
                           const std::string& delims = " \t\n\r",
                           std::string::size_type start = 0,
                           std::string::size_type end = std::string::npos);
        ~Tokenizer() noexcept = default;

        // Methods needed for the ForwardIterator access. Note that the iterators are
        // written in terms of hasAnother() and next(), so you should use one or the
        // other approaches, but not both at the same time.

        /*!
         Returns true if there is another item available and false otherwise.
         */
        inline bool hasAnother() const noexcept {
            return (_lastPos <= _end);
        }

        /*!
         Retrieves the next token and places it in token. It is invalid to call this
         if there are no more tokens.
         @return a reference to token
         */
        std::string& next(std::string& token);

        /*!
         Iterator based access. Note that if you combine this and the stream based access
         you will find that some tokens are "skipped" as these iterators are written in
         terms of the stream based access.
         */
        iterator begin() { return iterator(*this); }
        iterator end()   { return iterator(); }

    private:
        std::string             _s;
        std::string             _delim;
        std::string::size_type  _lastPos;
        std::string::size_type  _end;
    };

}}}

#endif
