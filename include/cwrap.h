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

#include <functional>
#include <memory>
#include <type_traits>

namespace cwrap {

namespace guard {

/*
* Use-cases:
*
* 2.
*
* (b) In a variant of this, a pointer is returned as an argument pased in:
*      struct some_type_t **ptr;
*      int error = do_init_stuff(ptr, bla, blabla);
*      if (error) {
*          //handle error
*      }
*      free_func(*ptr);
*
*/

template <class T>
struct DefaultFreePolicy {
    void operator() (std::remove_reference_t<T> &ptr) const {
        (void) ptr;
    }
};

template <class T>
struct ByValueStoragePolicy {
    using StorageType = std::remove_reference_t<T>;

    inline static const std::add_lvalue_reference_t<T> getFrom(const StorageType &t) {
        return t;
    }

    inline static std::add_lvalue_reference_t<T> getFrom(StorageType &t) {
        return t;
    }

    template <class ...Args>
    inline static StorageType createFrom(Args &&...args) {
        return std::remove_reference_t<T>(std::forward<Args>(args)...);
    }
};

template <class T> //TODO deleter
struct UniquePointerStoragePolicy {
    using StorageType = std::unique_ptr<std::remove_reference_t<T>>;

    inline static const std::add_lvalue_reference_t<T> getFrom(const StorageType &t) {
        return *t;
    }

    inline static std::add_lvalue_reference_t<T> getFrom(StorageType &t) {
        return *t;
    }

    template <class ...Args>
    inline static StorageType createFrom(Args&& ...args) {
        return std::make_unique<std::remove_reference_t<T>>(std::forward<Args>(args)...);
    }
};

template <class Type,
         class FreePolicy=DefaultFreePolicy<Type>,
         class StoragePolicy=ByValueStoragePolicy<Type>>
class Guard {
    public:

        template <typename = std::enable_if<
            std::is_default_constructible<FreePolicy>::value>>
        Guard()
            : _guarded { StoragePolicy::createFrom() }
            , _freeFunc {} {}

        Guard(FreePolicy func)
            : _guarded { StoragePolicy::createFrom() }
            , _freeFunc { func } {}

        ~Guard() {
            _freeFunc(StoragePolicy::getFrom(_guarded));
        }

        const Type &get() const {
            return StoragePolicy::getFrom(_guarded);
        }

        Type &get() {
            return StoragePolicy::getFrom(_guarded);
        }

        template <class T, class F, class M>
        friend Guard<T, F, M> make_guarded(F, T);

    private:
        typename StoragePolicy::StorageType _guarded;
        FreePolicy _freeFunc;

        Guard(FreePolicy func, Type t)
            : _guarded { StoragePolicy::createFrom(t) }
            , _freeFunc { func } {}

        template <typename = std::enable_if<
            std::is_default_constructible<FreePolicy>::value>>
        Guard(Type t)
            : _guarded { StoragePolicy::createFrom(t) }
            , _freeFunc {} {}
};

template <class T,
         class FreePolicy=DefaultFreePolicy<T>,
         class StoragePolicy=ByValueStoragePolicy<T>>
Guard<T, FreePolicy, StoragePolicy> make_guarded(FreePolicy policy, T t) {
    return Guard<std::remove_reference_t<T>, FreePolicy, StoragePolicy> { policy, t };
}

template <class T,
         class FreePolicy=DefaultFreePolicy<T>,
         class StoragePolicy=ByValueStoragePolicy<T>>
Guard<T, FreePolicy, StoragePolicy> make_guarded(T t) {
    return Guard<std::remove_reference_t<T>, FreePolicy, StoragePolicy> { t };
}

}

}
