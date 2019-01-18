//
//  _contract.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-11-17.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//
//  "borrowed" from ksscontract
//

#ifndef kssio_contract_hpp
#define kssio_contract_hpp

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

        void performThrowingCheck(const char* conditionType,
                                  const kss::util::_private::contract::_private::Expression& exp);

        void performTerminatingCheck(const char* conditionType,
                                     const kss::util::_private::contract::_private::Expression& exp);
    }


    /*!
     This macro is used to create the Expression objects used as inputs to the
     condition checking methods of this file.
     */
#   define KSS_EXPR(expr) kss::util::_private::contract::_private::Expression {(expr), #expr, __PRETTY_FUNCTION__, __FILE__, __LINE__}


    /*!
     The parameter check is a form of precondition that is presumed to be checking
     the incoming parameter arguments. One difference between this and the other
     conditions, is that instead of terminating execution, this throws an
     exception.

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. (In
     debug mode this will be checked via assertions.)

     Example:
         void myfn(int minValue, int maxValue) {
             parameter(KSS_EXPR(minValue <= maxValue));
             ... your function code ...
         }

     @throws std::invalid_argument if the expression fails
     @throws any other exception that the expression may throw
     */
    inline void parameter(const _private::Expression& exp) {
        _private::performThrowingCheck("Parameter", exp);
    }

    /*!
     This is a short-hand way of calling parameter multiple times.

     Typically you would not call this by creating the expression objects manually.
     Instead you should create the expression objects using the KSS_EXPR macro.

     Example:
         void myfn(int minValue, int maxValue) {
             parameters({
                 KSS_EXPR(minValue > 0),
                 KSS_EXPR(minValue <= maxValue)
             });
             ... function code ...
         }

     @throws std::invalid_argument (actually kss::contract::InvalidArgument) if one or
            more of the expressions fail
     @throws any other exception that the expressions may throw
     */
    inline void parameters(std::initializer_list<_private::Expression> exps) {
        for (const auto& exp : exps) {
            parameter(exp);
        }
    }


    /*!
     Check that a precondition is true. If the expression is not true, a message is written
     to cerr, and the program is terminated via terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. (In
     debug mode this will be checked via assertions.)

     Example:
         void myfn(int minValue, int maxValue) {
             precondition(KSS_EXPR(minValue <= maxValue));
             ... your function code ...
         }

     @throws any exception that the expression may throw
     */
    inline void precondition(const _private::Expression& exp) {
        _private::performTerminatingCheck("Precondition", exp);
    }

    /*!
     This is a short-hand way of calling precondition multiple times.

     Typically you would not call this by creating the expression objects manually.
     Instead you should create the expression objects using the KSS_EXPR macro.

     Example:
         void myfn(int minValue, int maxValue) {
             preconditions({
                 KSS_EXPR(minValue > 0),
                 KSS_EXPR(minValue <= maxValue);
             });
             ... your function code ...
         }

     @throws any exception that the expressions may throw
     */
    inline void preconditions(std::initializer_list<_private::Expression> exps) {
        for (const auto& exp : exps) {
            precondition(exp);
        }
    }


    /*!
     Check that a condition is true. If the expression is not true, a message is written
     to cerr, and the program is terminated via terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. (In
     debug mode this will be checked via assertions.)

     Example:
         void myfn(int minValue, int maxValue) {
             ... your function code ...
             condition(KSS_EXPR(minValue <= maxValue));
             ... your function code ...
         }

     @throws any exception that the expression may throw
     */
    inline void condition(const _private::Expression& exp) {
        _private::performTerminatingCheck("Condition", exp);
    }

    /*!
     This is a short-hand way of calling condition multiple times.

     Typically you would not call this by creating the expression objects manually.
     Instead you should create the expression objects using the KSS_EXPR macro.

     Example:
         void myfn(int minValue, int maxValue) {
             ... your function code ...
             conditions({
                 KSS_EXPR(minValue > 0),
                 KSS_EXPR(minValue <= maxValue)
             });
             ... your function code ...
         }

     @throws std::invalid_argument (actually kss::contract::InvalidArgument) if one or
            more of the expressions fail.
     @throws any exception that the expressions may throw
     */
    inline void conditions(std::initializer_list<_private::Expression> exps) {
        for (const auto& exp : exps) {
            precondition(exp);
        }
    }


    /*!
     Check that a postcondition is true. If the expression is not true, a message is written
     to cerr, and the program is terminated via terminate().

     Typically you would not call this by creating the expression object manually.
     Instead you should create the expression object using the KSS_EXPR macro. (In
     debug mode this will be checked via assertions.)

     Example:
         void myfn(int minValue, int maxValue) {
             ... your function code ...
             postcondition(KSS_EXPR(minValue <= maxValue));
         }

     @throws any exception that the expression may throw
     */
    inline void postcondition(const _private::Expression& exp) {
        _private::performTerminatingCheck("Postcondition", exp);
    }

    /*!
     This is a short-hand way of calling postcondition multiple times.

     Typically you would not call this by creating the expression objects manually.
     Instead you should create the expression objects using the KSS_EXPR macro.

     Example:
         void myfn(int minValue, int maxValue) {
             ... your function code ...
             postconditions({
                 KSS_EXPR(minValue > 0),
                 KSS_EXPR(minValue <= maxValue)
             });
         }

     @throws any exception that the expressions may throw
     */
    inline void postconditions(std::initializer_list<_private::Expression> exps) {
        for (const auto& exp : exps) {
            postcondition(exp);
        }
    }

}}}}

#endif
