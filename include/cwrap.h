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
    using RawType = std::remove_reference_t<T>;
    using StorageType = RawType;

    inline static const RawType& getFrom(const StorageType &t) {
        return t;
    }

    inline static RawType& getFrom(StorageType &t) {
        return t;
    }

    template <class ...Args>
    inline static StorageType createFrom(Args &&...args) {
        return RawType(std::forward<Args>(args)...);
    }
};

template <class Type,
     class FreePolicy=DefaultFreePolicy<Type>,
     class MemoryPolicy=DefaultMemoryPolicy<Type>
     >
class Guard {
public:

    Guard()
        : _guarded { MemoryPolicy::createFrom() }
        , _freeFunc {} {}

    Guard(FreePolicy func)
        : _guarded { MemoryPolicy::createFrom() }
        , _freeFunc { func } {}

    ~Guard() {
        _freeFunc(MemoryPolicy::getFrom(_guarded));
    }

    const Type &get() const {
        return MemoryPolicy::getFrom(_guarded);
    }

    Type &get() {
        return MemoryPolicy::getFrom(_guarded);
    }

    template <class T, class F, class M>
    friend Guard<T,F,M> make_guarded(F, T);

private:
    typename MemoryPolicy::StorageType _guarded;
    FreePolicy _freeFunc;

    Guard(FreePolicy func, Type t)
        : _guarded { MemoryPolicy::createFrom(t) }
        , _freeFunc { func } {}

    Guard(Type t)
        : _guarded { MemoryPolicy::createFrom(t) }
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
