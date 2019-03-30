//
//  raii.hpp
//
//  Created by Steven W. Klassen on 2016-08-26.
//  Copyright © 2016 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssutil_raii_h
#define kssutil_raii_h

#include <functional>

namespace kss { namespace util {

    /*!
     RAII (Resource acquisition is initialization) is used to implement the RAII
     pattern without always having to create a custom class.

     You give this class two lambdas. The first is the setup code which is run
     by the class constructor. The second is the cleanup code which is run by the
     class destructor. Hence the RAII pattern, the setup runs when the object
     comes into scope and the cleanup when it goes out of scope.

     This is structured like the following:

     @code
     {
         RAII myRAII([]{
            cout << "this gets run at setup" << endl;
         }, []{
            cout << "this gets run at cleanup" << endl;
         });

         cout << "this is my application code" << endl;
     }
     @endcode

     This would produce the following output:

     @code
     this gets run at setup
     this is my application code
     this gets run at cleanup
     @endcode
     */
    class RAII {
    public:
        typedef std::function<void()> lambda_t;

        /**
         Construct an instance of the guard giving it a lambda. Note that the init lambda
         will execute immediately (in the constructor) while the cleanup lambda will be
         executed in the destructor.
         */
        RAII(const lambda_t& initCode, const lambda_t& cleanupCode) : _cleanupCode(cleanupCode) {
            initCode();
        }

        RAII(const lambda_t& initCode, lambda_t&& cleanupCode) : _cleanupCode(move(cleanupCode)) {
            initCode();
        }

        ~RAII() {
            _cleanupCode();
        }

        RAII& operator=(const RAII&) = delete;
        RAII& operator=(RAII&&) = delete;

    private:
        lambda_t _cleanupCode;
    };


    /**
     Provide a "finally" block. This is a shortcut to an RAII that contains no
     initialization code.
     */
    class Finally : public RAII {
    public:
        explicit Finally(const lambda_t& code) : RAII([]{}, code) {}
        explicit Finally(lambda_t&& code) : RAII([]{}, move(code)) {}
    };

}}

#endif
