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

#include <utility>

namespace cwrap {

namespace error {

template <class Functor,
         class Rv,
         class ...Args>
class CallGuard {
    public:
        template <class T>
        CallGuard(T &&t)
            : _functor { std::forward<T>(t) } {}

        template <class T=Functor,
                 typename = std::enable_if_t<std::is_default_constructible<T>::value>>
        CallGuard()
            : _functor {} {}

        Rv operator() (Args&& ...args) {
            return _functor(std::forward<Args>(args)...);
        }
    private:
        Functor _functor;
};

} //::error

} //::cwrap
