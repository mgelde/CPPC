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
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace cwrap {

namespace guard {

template <class FreePolicy>
struct IsNoexcept {
private:
    /**
     * This auxiliary template get the arguments of FreePolicy::operator()
     * so that we can write a well-formed invocation of operator() to verify
     * that it is noexcept.
     */
    template <class F>
    struct _is_invocation_expression_noexcept : public std::false_type {};

    template <class T, class F, class... Args>
    struct _is_invocation_expression_noexcept<T (F::*)(Args...)> {
        static constexpr bool value{noexcept(std::declval<FreePolicy>()(std::declval<Args>()...))};
    };
    template <class T, class F, class... Args>
    struct _is_invocation_expression_noexcept<T (F::*)(Args...) const> {
        static constexpr bool value{noexcept(std::declval<FreePolicy>()(std::declval<Args>()...))};
    };

public:
    constexpr static bool value{_is_invocation_expression_noexcept<decltype(
            &std::decay_t<FreePolicy>::operator())>::value};
};

/**
 * Function pointers are by default treated as noexcept(false).
 *
 * C++14 does not distinguish between function types of the form
 *  T(Args...) noexcept
 * and
 *  T(Args...)
 * where T and Args... are types.
 *
 * In the case of function pointers, where we cannot, at compile time,
 * know the pointee, we must assume noexcept(false).
 */
template <class Rv, class... Args>
struct IsNoexcept<Rv (*)(Args...)> : public std::false_type {};

template <class T>
struct ByValueStoragePolicy {
    using StorageType = std::remove_reference_t<T>;

    inline static std::add_lvalue_reference_t<const StorageType> getFrom(
            const StorageType &t) noexcept {
        return t;
    }

    inline static std::add_lvalue_reference_t<StorageType> getFrom(StorageType &t) noexcept {
        return t;
    }

    template <class... Args>
    inline static StorageType createFrom(Args &&... args) noexcept(noexcept(StorageType{
            std::forward<Args>(args)...})) {
        return StorageType{std::forward<Args>(args)...};
    }
};

template <class T>  // TODO deleter
struct UniquePointerStoragePolicy {
    using RawType = std::remove_reference_t<T>;
    using StorageType = std::unique_ptr<std::remove_reference_t<T>>;

    inline static std::add_lvalue_reference_t<const RawType> getFrom(
            const StorageType &t) noexcept {
        return *t;
    }

    inline static std::add_lvalue_reference_t<RawType> getFrom(StorageType &t) noexcept {
        return *t;
    }

    template <class... Args>
    inline static StorageType createFrom(Args &&... args) {
        return std::make_unique<RawType>(std::forward<Args>(args)...);
    }
};

template <class T>
using _PointerOrRefType =
        std::conditional_t<std::is_pointer<T>::value, T, std::add_lvalue_reference_t<T>>;

template <class T>
using _FreePolicyFunctionType = void(_PointerOrRefType<T>);

template <class T>
using DefaultFreePolicy = std::function<_FreePolicyFunctionType<T>>;

template <class Type,  // what if: Type is const, Type is volatile, type is
                       // pointer to const?
          class FreePolicy = DefaultFreePolicy<Type>,
          class StoragePolicy = ByValueStoragePolicy<Type>>
class Guard {
    static_assert(!std::is_reference<Type>::value, "Cannot guard references");

private:
    using _RawType = std::decay_t<Type>;

public:
    template <class F = FreePolicy,
              typename = std::enable_if_t<std::is_default_constructible<F>::value>>
    Guard() : _guarded{StoragePolicy::createFrom()}, _freeFunc{} {}

    Guard(std::remove_reference_t<FreePolicy> &&func)
            : _guarded{StoragePolicy::createFrom()}, _freeFunc{std::move(func)} {}

    // if the FreePolicy is a reference, we need to initialize a potential
    // non-const ref from a non-const ref
    Guard(std::conditional_t<std::is_reference<FreePolicy>::value, FreePolicy, const FreePolicy &>
                  func)
            : _guarded{StoragePolicy::createFrom()}, _freeFunc{func} {}

    Guard(std::conditional_t<std::is_reference<FreePolicy>::value, FreePolicy, const FreePolicy &>
                  func,
          const _RawType &t)
            : _guarded{StoragePolicy::createFrom(t)}, _freeFunc{func} {}

    Guard(std::remove_reference_t<FreePolicy> &&func, const _RawType &t)
            : _guarded{StoragePolicy::createFrom(t)}, _freeFunc{std::move(func)} {}

    template <class F = FreePolicy,
              typename = std::enable_if_t<std::is_default_constructible<F>::value>>
    Guard(const _RawType &t) : _guarded{StoragePolicy::createFrom(t)}, _freeFunc{} {}

    Guard(const Guard &) = delete;

    Guard(Guard &&other);

    Guard &operator=(const Guard &) = delete;

    Guard &operator=(Guard &&other);

    /**@brief Release the resource held by this guard
     *
     * This function is noexcept if and only if all of the following hold:
     * - The underlying free policy has an operator that is marked noexcept.
     * - The StoragePolicy has implemented a getFrom method that is noexcept.
     *
     * Note that C functions never emit exceptions. It is therefore safe to
     * declare the operator() of the FreePolicy noexcept if it only uses C
     * functions.
     */
    ~Guard() noexcept(IsNoexcept<FreePolicy>::value);

    const Type &get() const;
    Type &get();

private:
    typename StoragePolicy::StorageType _guarded;
    bool _released{false};
    FreePolicy _freeFunc;
    inline void _releaseIfNecessary() noexcept(IsNoexcept<FreePolicy>::value);
};

template <class Type, class FreePolicy, class StoragePolicy>
Guard<Type, FreePolicy, StoragePolicy>::Guard(Guard &&other)
        : _guarded{std::move(other._guarded)}, _freeFunc{std::move(other._freeFunc)} {
    other._released = true;
}

template <class Type, class FreePolicy, class StoragePolicy>
inline void Guard<Type, FreePolicy, StoragePolicy>::_releaseIfNecessary() noexcept(
        IsNoexcept<FreePolicy>::value) {
    if (!_released) {
        _freeFunc(StoragePolicy::getFrom(_guarded));
    }
}
template <class Type, class FreePolicy, class StoragePolicy>
const Type &Guard<Type, FreePolicy, StoragePolicy>::get() const {
    return StoragePolicy::getFrom(_guarded);
}

template <class Type, class FreePolicy, class StoragePolicy>
Type &Guard<Type, FreePolicy, StoragePolicy>::get() {
    return StoragePolicy::getFrom(_guarded);
}

template <class Type, class FreePolicy, class StoragePolicy>
Guard<Type, FreePolicy, StoragePolicy>::~Guard() noexcept(IsNoexcept<FreePolicy>::value) {
    _releaseIfNecessary();
}

template <class Type, class FreePolicy, class StoragePolicy>
Guard<Type, FreePolicy, StoragePolicy> &Guard<Type, FreePolicy, StoragePolicy>::operator=(
        Guard &&other) {
    _releaseIfNecessary();
    _released = false;
    _freeFunc = std::move(other._freeFunc);
    _guarded = std::move(other._guarded);
    other._released = true;
    return *this;
}

}  // namesapce guard
}  // namepsace cwrap
