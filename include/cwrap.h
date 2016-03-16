#pragma once

#include <functional>

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
struct DefaultMemoryPolicy {
    using StorageType = std::remove_reference_t<T>;
};

template <class Type,
     class FreePolicy=DefaultFreePolicy<Type>,
     class MemoryPolicy=DefaultMemoryPolicy<Type>
     >
class Guard {
public:

    Guard()
        : _guarded {}
        , _freeFunc {} {}

    Guard(FreePolicy func)
        : _guarded {}
        , _freeFunc { func } {}

    ~Guard() {
        _freeFunc(_guarded);
    }

    const Type &get() const {
        return _guarded;
    }

    Type &get() {
        return _guarded;
    }

    template <class T, class F, class M>
    friend Guard<T,F,M> make_guarded(F, T);

private:
    typename MemoryPolicy::StorageType _guarded;
    FreePolicy _freeFunc;

    Guard(FreePolicy func, Type t)
        : _guarded { t }
        , _freeFunc { func } {}

    Guard(Type t)
        : _guarded { t }
        , _freeFunc {} {}
};

template <class T,
     class FreePolicy=DefaultFreePolicy<T>,
     class MemoryPolicy=DefaultMemoryPolicy<T>>
Guard<T, FreePolicy, MemoryPolicy> make_guarded(FreePolicy policy, T t) {
    return Guard<std::remove_reference_t<T>, FreePolicy, MemoryPolicy> { policy, t };
}

template <class T,
     class FreePolicy=DefaultFreePolicy<T>,
     class MemoryPolicy=DefaultMemoryPolicy<T>>
Guard<T, FreePolicy, MemoryPolicy> make_guarded(T t) {
    return Guard<std::remove_reference_t<T>, FreePolicy, MemoryPolicy> { t };
}
}

}
