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

#include <cerrno>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "boost/format.hpp"

namespace cwrap {

namespace _auxiliary {

template <class T>
using EnableIfIsPrecallFunc = std::enable_if_t<std::is_same<T, void()>::value>;

template <class T, class = EnableIfIsPrecallFunc<decltype(T::preCall)>>
inline void _callIf(decltype(T::preCall) *) {
    T::preCall();
}

template <class... Ts>
inline void _callIf(Ts*...) {}

template <class T>
inline void callPrecCallIfPresent() {
    _callIf<T>(nullptr);
}

}  // ::_auxiliary

struct ReportReturnValueErrorPolicy {
    template <class Rv>
    static void handleError(const Rv& rv);
};

template <class Rv>
void ReportReturnValueErrorPolicy::handleError(const Rv& rv) {
    boost::format fmtr{"Return value indicated error: %d"};
    fmtr % rv;
    throw std::runtime_error(fmtr.str());
}

struct ErrnoErrorPolicy {
    template <class Rv>
    static void handleError(const Rv&);
};

template <class Rv>
void ErrnoErrorPolicy::handleError(const Rv&) {
    throw std::runtime_error(std::strerror(errno));
}

struct ErrorCodeErrorPolicy {
    template <class Rv>
    static void handleError(const Rv& rv);
};

template <class Rv>
void ErrorCodeErrorPolicy::handleError(const Rv& rv) {
    static_assert(std::is_integral<std::decay_t<Rv>>::value, "Must be an integral value");
    throw std::runtime_error(std::strerror(-rv));
}

using DefaultErrorPolicy = ReportReturnValueErrorPolicy;

struct IsZeroReturnCheckPolicy {
    template <class Rv>
    static inline bool returnValueIsOk(const Rv& rv) {
        static_assert(std::is_integral<std::decay_t<Rv>>::value, "Must be an integral value");
        return rv == 0;
    }
};

struct IsNotNegativeReturnCheckPolicy {
    template <class Rv>
    static inline bool returnValueIsOk(const Rv& rv) {
        static_assert(std::is_integral<std::decay_t<Rv>>::value, "Must be an integral value");
        static_assert(std::is_signed<std::decay_t<Rv>>::value, "Must be a signed type");

        return rv >= 0;
    }
};

struct IsNotZeroReturnCheckPolicy {
    template <class Rv>
    static inline bool returnValueIsOk(const Rv& rv) {
        static_assert(std::is_integral<std::decay_t<Rv>>::value, "Must be an integral value");
        static_assert(std::is_signed<std::decay_t<Rv>>::value, "Must be a signed type");

        return rv != 0;
    }
};

struct IsNotNullptrReturnCheckPolicy {
    template <class Rv>
    static inline bool returnValueIsOk(const Rv& rv) {
        return nullptr != rv;
    }
};

struct IsErrnoZeroReturnCheckPolicy {
    template <class Rv>
    static inline bool returnValueIsOk(const Rv&) {
        return errno == 0;
    }
    static inline void preCall() { errno = 0; }
};

using DefaultReturnCheckPolicy = IsZeroReturnCheckPolicy;

template <class R = DefaultReturnCheckPolicy,
          class E = DefaultErrorPolicy,
          class Callable = std::function<void(void)>,
          class... Args>
inline auto callChecked(Callable&& callable, Args&&... args) {
    ::cwrap::_auxiliary::callPrecCallIfPresent<R>();
    const auto retVal = callable(std::forward<Args>(args)...);
    if (!R::returnValueIsOk(retVal)) {
        E::handleError(retVal);
    }
    return retVal;
}

template <class Functor,
          class ReturnCheckPolicy = DefaultReturnCheckPolicy,
          class ErrorPolicy = DefaultErrorPolicy>
class CallGuard {
private:
    using FunctorOrFuncRefType = std::conditional_t<std::is_function<Functor>::value,
                                                    std::add_lvalue_reference_t<Functor>,
                                                    Functor>;

public:
    template <class T>
    CallGuard(T&& t) : _functor{std::forward<T>(t)} {}

    template <class T = Functor,
              typename = std::enable_if_t<std::is_default_constructible<T>::value>>
    CallGuard() : _functor{} {}

    template <class... Args>
    auto operator()(Args&&... args) {
        return callChecked<ReturnCheckPolicy, ErrorPolicy>(_functor, std::forward<Args>(args)...);
    }

private:
    FunctorOrFuncRefType _functor;
};

template <class ReturnCheckPolicy = DefaultReturnCheckPolicy,
          class ErrorPolicy = DefaultErrorPolicy>
class CallCheckContext {
public:
    template <class Callable, class... Args>
    static inline auto callChecked(Callable&& callable, Args&&... args) {
        return ::cwrap::callChecked<ReturnCheckPolicy, ErrorPolicy>(
                std::forward<Callable>(callable), std::forward<Args>(args)...);
    }
};

}  // namespace cwrap
