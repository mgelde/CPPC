/*
 *  CWrap - A library to use C-code in C++.
 *  Copyright (C) 2016  Marcus Gelderie
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#pragma once

#include <stdexcept>
#include <functional>
#include <utility>
#include <type_traits>

namespace cwrap {

namespace error {

template <class Rv>
struct DefaultErrorPolicy {

    static inline void handleError(std::remove_reference_t<Rv>&) {
                throw std::runtime_error(
                        std::string("Return value indicated error"));
            }

};

template <class Rv>
struct IsZeroReturnCheckPolicy {
    static_assert(std::is_integral<std::remove_reference_t<Rv>>::value,
            "Must be an integral value");

    static inline bool returnValueIsOk(const std::remove_reference_t<Rv> &rv) {
        return rv == 0;
    }

};

template <class Rv>
struct IsNotNegativeReturnCheckPolicy {
    static_assert(std::is_integral<std::remove_reference_t<Rv>>::value,
            "Must be an integral value");
    static_assert(std::is_signed<std::remove_reference_t<Rv>>::value,
            "Must be a signed type");

    static inline bool returnValueIsOk(const std::remove_reference_t<Rv> &rv) {
        return rv >= 0;
    }

};

template <class Functor,
         class Rv,
         class ReturnCheckPolicy=IsZeroReturnCheckPolicy<Rv>,
         class ErrorPolicy=DefaultErrorPolicy<Rv>>
class CallGuard {
    public:
        template <class T>
        CallGuard(T &&t)
            : _functor { std::forward<T>(t) } {}

        template <class T=Functor,
                 typename = std::enable_if_t<std::is_default_constructible<T>::value>>
        CallGuard()
            : _functor {} {}

        template <class ...Args>
        Rv operator() (Args&& ...args) {
            Rv retVal = _functor(std::forward<Args>(args)...);
            if (!ReturnCheckPolicy::returnValueIsOk(retVal)) {
                ErrorPolicy::handleError(retVal);
            }
            return retVal;
        }
    private:
        Functor _functor;
};

} //::error

} //::cwrap
