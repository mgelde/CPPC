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

namespace cwrap {

namespace error {

struct ReportReturnValueErrorPolicy {
    template <class Rv>
    static inline void handleError(const Rv& rv) {
        std::ostringstream stream{"Return value indicated error. Got: "};
        stream << rv;
        throw std::runtime_error(stream.str());
    }
};

struct ErrnoErrorPolicy {
    template <class Rv>
    static inline void handleError(const Rv&) {
        throw std::runtime_error(std::strerror(errno));
    }
};

struct ErrorCodeErrorPolicy {
    static inline void handleError(int rv) { throw std::runtime_error(std::strerror(-rv)); }
};

using DefaultErrorPolicy = ReportReturnValueErrorPolicy;

struct IsZeroReturnCheckPolicy {
    template <class Rv>
    static inline bool returnValueIsOk(const Rv& rv) {
        static_assert(std::is_integral<std::remove_reference_t<Rv>>::value,
                      "Must be an integral value");

        return rv == 0;
    }
};

struct IsNotNegativeReturnCheckPolicy {
    template <class Rv>
    static inline bool returnValueIsOk(const Rv& rv) {
        static_assert(std::is_integral<std::remove_reference_t<Rv>>::value,
                      "Must be an integral value");
        static_assert(std::is_signed<std::remove_reference_t<Rv>>::value, "Must be a signed type");

        return rv >= 0;
    }
};

struct IsNotZeroReturnCheckPolicy {
    template <class Rv>
    static inline bool returnValueIsOk(const Rv& rv) {
        static_assert(std::is_integral<std::remove_reference_t<Rv>>::value,
                      "Must be an integral value");
        static_assert(std::is_signed<std::remove_reference_t<Rv>>::value, "Must be a signed type");

        return rv != 0;
    }
};

using DefaultReturnCheckPolicy = IsZeroReturnCheckPolicy;

template <class R = DefaultReturnCheckPolicy,
          class E = DefaultErrorPolicy,
          class Callable=std::function<void(void)>,
          class... Args>
inline auto CALL_CHECKED(Callable&& callable, Args&&... args) {
    const auto& retVal = callable(std::forward<Args>(args)...);
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
        return CALL_CHECKED<ReturnCheckPolicy, ErrorPolicy>(_functor, std::forward<Args>(args)...);
    }

private:
    FunctorOrFuncRefType _functor;
};


}  // namepsace error

}  // namespace cwrap
