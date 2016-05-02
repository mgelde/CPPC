/*
 *  CWrap - A library to use C-code in C++.
 *  Copyright (C) 2016  Marcus Gelderie


 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.

 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#pragma once

#include <memory>
#include <exception>

#include "gtest/gtest.h"

namespace cwrap {

namespace testing {

namespace mock {

class MockException : public std::exception {
    public:
        const char *what() const noexcept override {
            return "The mock was instrumented incorrectly";
        }
};

class some_type_t {
    public:

        bool isFreed() const {
            return freed && released;
        }

        void reset() {
            freed = false;
            released = false;
        }

        friend struct release_resources_operator;
        friend struct free_resources_operator;
        friend struct do_init_work_operator;
        friend struct create_and_initialize_operator;

    private:
        bool released = false;
        bool freed = false;
};

static some_type_t __some_type {};

void do_init_work(some_type_t *ptr) {
    if (ptr) {
        ptr->reset();
    }
}

some_type_t *create_and_initialize() {
    do_init_work(&__some_type);
    return &__some_type;
}

void release_resources(some_type_t *ptr);
void free_resources(some_type_t *ptr);

struct __mock_call_operator {
    public:
        bool called() const {
            return _called;
        }
        unsigned int numCalls() const {
            return _numCalled;
        }
    protected:
        bool _called { false };
        unsigned int _numCalled { 0 };
};

struct release_resources_operator : public __mock_call_operator {
    void operator() (some_type_t *ptr) {
        ptr->released = true;
        _called = true;
        _numCalled++;
    }
};

struct free_resources_operator : public __mock_call_operator {
    void operator() (some_type_t *ptr) {
        if (ptr != &__some_type) {
            throw MockException {};
        }
        release_resources(ptr);
        ptr->freed = true;
        _called = true;
        _numCalled++;
    }
};

struct some_func_with_error_code_operator : public __mock_call_operator {
    int operator() (int errorCode) {
        _called = true;
        _numCalled++;
        return errorCode;
    }
};

class MockAPI {
    public:
        const free_resources_operator &freeResourcesFunc() const {
            return _freeFunc;
        }

        const release_resources_operator &releaseResourcesFunc() const {
            return _releaseFunc;
        }

        const some_func_with_error_code_operator &someFuncWithErrorCode() const {
            return _someFuncWithErrorCode;
        }

        void reset() {
            _freeFunc = free_resources_operator {};
            _releaseFunc = release_resources_operator {};
            _someFuncWithErrorCode = some_func_with_error_code_operator {};
        }

        void doFreeResources(some_type_t *ptr) {
            _freeFunc(ptr);
        }

        void doReleaseResources(some_type_t *ptr) {
            _releaseFunc(ptr);
        }

        int doSomeFuncWithErrorCode(int errorCode) {
            return _someFuncWithErrorCode(errorCode);
        }

        static MockAPI &instance() {
            return *_instance;
        }

    private:
        free_resources_operator _freeFunc;
        release_resources_operator _releaseFunc;
        some_func_with_error_code_operator _someFuncWithErrorCode;

        static std::unique_ptr<MockAPI> _instance;
};

std::unique_ptr<MockAPI> MockAPI::_instance {std::make_unique<MockAPI>()};

void free_resources(some_type_t *ptr) {
    MockAPI::instance().doFreeResources(ptr);
}

void release_resources(some_type_t *ptr) {
    MockAPI::instance().doReleaseResources(ptr);
}

int some_func_with_error_code(int errorCode) {
    return MockAPI::instance().doSomeFuncWithErrorCode(errorCode);
}

}

namespace assertions {

template <class T>
void ASSERT_CALLED(T &&t) {
    ASSERT_TRUE(t.called());
}

template <class T>
void ASSERT_NOT_CALLED(T &&t) {
    ASSERT_FALSE(t.called());
}

template <class T>
void ASSERT_NUM_CALLED(T &&t, unsigned int n) {
    ASSERT_EQ(t.numCalls(), n);
}

}

}



}
