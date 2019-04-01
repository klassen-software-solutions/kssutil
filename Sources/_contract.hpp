//
//  _contract.hpp
//  ksscontract
//
//  Created by Steven W. Klassen on 2018-11-17.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
// "Borrowed" from ksscontract
//

#ifndef kssutil_contract_hpp
#define kssutil_contract_hpp

#include <initializer_list>
#include <string>

namespace kss { namespace util { namespace _private { namespace contract {

    namespace _private {
        struct Expression {
            bool        result = false;
            const char* expr = nullptr;
            const char* functionName = nullptr;
            const char* fileName = nullptr;
            unsigned    lineNo = 0;
        };

        void performThrowingCheck(const char* conditionType, const Expression& exp);
        void performTerminatingCheck(const char* conditionType, const Expression& exp);

        inline void parameter(const Expression& exp) {
            performThrowingCheck("Parameter", exp);
        }

        inline void precondition(const Expression& exp) {
            performTerminatingCheck("Precondition", exp);
        }

        inline void condition(const Expression& exp) {
            performTerminatingCheck("Condition", exp);
        }

        inline void postcondition(const Expression& exp) {
            performTerminatingCheck("Postcondition", exp);
        }
    }


    /*!
     This macro is used to create the Expression objects used as inputs to the
     condition checking methods of this library.
     */
#   define KSS_EXPR(expr) kss::util::_private::contract::_private::Expression {(expr), #expr, __PRETTY_FUNCTION__, __FILE__, __LINE__}


    /*!
     The parameter check is a form of precondition that is presumed to be checking
     the incoming parameter arguments. One difference between this and the other
     conditions, is that instead of terminating execution, this throws an
     exception.

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro.

     Example:
     @code
     void myfn(int minValue, int maxValue) {
         parameters({
             KSS_EXPR(minValue > 0),
             KSS_EXPR(minValue <= maxValue)
         });
         ... function code ...
     }
     @endcode

     @throws std::invalid_argument if one or more of the expressions fail
     @throws any other exception that the expressions may throw
     */
    inline void parameters(std::initializer_list<_private::Expression> exps) {
        for (const auto& exp : exps) {
            _private::parameter(exp);
        }
    }


    /*!
     Check that a precondition is true. If an expression is not true, a message is written
     to std::cerr, and the program is terminated via std::terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro.

     Example:
     @code
     void myfn(int minValue, int maxValue) {
         preconditions({
             KSS_EXPR(minValue > 0),
             KSS_EXPR(minValue <= maxValue)
         });
         ... your function code ...
     }
     @endcode

     @throws any exception that the expressions may throw
     */
    inline void preconditions(std::initializer_list<_private::Expression> exps) {
        for (const auto& exp : exps) {
            _private::precondition(exp);
        }
    }


    /*!
     Check that a condition is true. If an expression is not true, a message is written
     to std::cerr, and the program is terminated via std::terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro.

     Example:
     @code
     void myfn(int minValue, int maxValue) {
         ... your function code ...
         conditions({
             KSS_EXPR(minValue > 0),
             KSS_EXPR(minValue <= maxValue)
         });
         ... your function code ...
     }
     @endcode

     @throws std::invalid_argument (actually kss::contract::InvalidArgument) if one or
            more of the expressions fail.
     @throws any exception that the expressions may throw
     */
    inline void conditions(std::initializer_list<_private::Expression> exps) {
        for (const auto& exp : exps) {
            _private::condition(exp);
        }
    }


    /*!
     Check that a postcondition is true. If an expression is not true, a message is written
     to cerr, and the program is terminated via terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. 

     Example:
     @code
     void myfn(int minValue, int maxValue) {
         ... your function code ...
         postconditions({
             KSS_EXPR(minValue > 0),
             KSS_EXPR(minValue <= maxValue)
         });
     }
     @endcode

     @throws any exception that the expressions may throw
     */
    inline void postconditions(std::initializer_list<_private::Expression> exps) {
        for (const auto& exp : exps) {
            _private::postcondition(exp);
        }
    }

}}}}

#endif
