/*   Copyright 2016-2019 Marcus Gelderie
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace cppc {

namespace _auxiliary {

/**
 * @brief Struct to determine if a functor is noexcept.
 *
 * A functor may define an operator() to be either noexcept(true)
 * or noexcept(false). This helper template exposes whether the
 * operator is noeexcept or not.
 */
template <class FreePolicy>
struct IsNoexcept {
private:
    /**
     * This auxiliary template gets the arguments of FreePolicy::operator()
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
#if __cplusplus >= 201703L
    template <class T, class F, class... Args>
    struct _is_invocation_expression_noexcept<T (F::*)(Args...) noexcept> {
        static constexpr bool value{noexcept(std::declval<FreePolicy>()(std::declval<Args>()...))};
    };
    template <class T, class F, class... Args>
    struct _is_invocation_expression_noexcept<T (F::*)(Args...) const noexcept> {
        static constexpr bool value{noexcept(std::declval<FreePolicy>()(std::declval<Args>()...))};
    };
#endif

public:
    constexpr static bool value{_is_invocation_expression_noexcept<decltype(
            &std::decay_t<FreePolicy>::operator())>::value};
};

#if __cplusplus < 201703L
/**
 * @brief Specialization for function pointers.
 *
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
#else
template <class Rv, class... Args>
struct IsNoexcept<Rv (*)(Args...)> : public std::false_type {};
template <class Rv, class... Args>
struct IsNoexcept<Rv (&)(Args...) noexcept> : public std::true_type {};
template <class Rv, class... Args>
struct IsNoexcept<Rv (&)(Args...)> : public std::false_type {};
template <class Rv, class... Args>
struct IsNoexcept<Rv (*)(Args...) noexcept> : public std::true_type {};
#endif

/**
 * Turn T into a reference-type, unless it's a pointer
 */
template <class T>
using PointerOrRefType =
        std::conditional_t<std::is_pointer<T>::value, T, std::add_lvalue_reference_t<T>>;

}  // namespace _auxiliary

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

template <class T>
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
    inline static StorageType createFrom(Args &&... args) noexcept(
            noexcept(std::make_unique<RawType>(std::forward<Args>(args)...))) {
        return std::make_unique<RawType>(std::forward<Args>(args)...);
    }
};

template <class T>
using _FreePolicyFunctionType = void(_auxiliary::PointerOrRefType<T>);

template <class T>
using DefaultFreePolicy = std::function<_FreePolicyFunctionType<T>>;

template <class Type,
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
    ~Guard() noexcept(_auxiliary::IsNoexcept<FreePolicy>::value);

    const Type &get() const;
    Type &get();

private:
    typename StoragePolicy::StorageType _guarded;
    bool _released{false};
    FreePolicy _freeFunc;
    inline void _releaseIfNecessary() noexcept(_auxiliary::IsNoexcept<FreePolicy>::value);
};

template <class Type, class FreePolicy, class StoragePolicy>
Guard<Type, FreePolicy, StoragePolicy>::Guard(Guard &&other)
        : _guarded{std::move(other._guarded)}, _freeFunc{std::move(other._freeFunc)} {
    other._released = true;
}

template <class Type, class FreePolicy, class StoragePolicy>
inline void Guard<Type, FreePolicy, StoragePolicy>::_releaseIfNecessary() noexcept(
        _auxiliary::IsNoexcept<FreePolicy>::value) {
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
Guard<Type, FreePolicy, StoragePolicy>::~Guard() noexcept(
        _auxiliary::IsNoexcept<FreePolicy>::value) {
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

}  // namespace cppc
