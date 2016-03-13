#pragma once

#include <functional>

namespace cwrap {

namespace guard {

template <class Type, class FreeFuncT=std::function<void(Type)>>
class Guard {
    public:

        template <class T, class F>
        Guard(T &&t, F &&func)
            : _guarded { std::forward<T>(t) }
            , _freeFunc { std::forward<F>(func) }
        {}

        ~Guard() {
            _freeFunc(_guarded);
        }

        Type &get() {
            return _guarded;
        }

        const Type &get() const {
            return _guarded;
        }

    private:
        Type _guarded;
        FreeFuncT _freeFunc;

};

}

}
