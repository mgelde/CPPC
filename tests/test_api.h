/*
 *   CWrap - A library to use C-code in C++.
 *   Copyright (C) 2016  Marcus Gelderie
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <exception>

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
        friend struct  create_and_initialize_operator;

    private:
        bool released = false;
        bool freed = false;
};

static some_type_t __some_type;

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
    protected:
        bool _called { false };
};

struct release_resources_operator : public __mock_call_operator {
    void operator() (some_type_t *ptr) {
        ptr->released = true;
        _called = true;
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

        void reset() {
            _freeFunc = free_resources_operator {};
            _releaseFunc = release_resources_operator {};
        }

        void doFreeResources(some_type_t *ptr) {
            _freeFunc(ptr);
        }

        void doReleaseResources(some_type_t *ptr) {
            _releaseFunc(ptr);
        }

        static MockAPI &instance() {
            return *_instance;
        }

    private:
        free_resources_operator _freeFunc;
        release_resources_operator _releaseFunc;

        static std::unique_ptr<MockAPI> _instance;
};

std::unique_ptr<MockAPI> MockAPI::_instance {std::make_unique<MockAPI>()};

void free_resources(some_type_t *ptr) {
    MockAPI::instance().doFreeResources(ptr);
}

void release_resources(some_type_t *ptr) {
    MockAPI::instance().doReleaseResources(ptr);
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

}

}



}
