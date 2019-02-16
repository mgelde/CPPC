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

#include <exception>
#include <memory>

#include "gtest/gtest.h"

/*
 * In order to test C code, we also use C name mangling
 *
 */
extern "C" {

int c_api_some_func_with_error_code(int code, int *ctx);

}

namespace cppc {

namespace testing {

namespace mock {

/** @brief Thrown when the mocks are used incorrectly
 *
 * This exception is thrown by the mock API below to indicate that the mock was
 * used incorrectly.
 */
class MockException : public std::exception {
public:
    const char *what() const noexcept override { return "The mock was instrumented incorrectly"; }
};

namespace api {

/** @brief a demo type to use in unit tests.
 *
 * This class serves as a sample type in the mock API we define below.  It
 * records the number the object is constructed or copied so we can assert that
 * the number of copies is what we expect it to be.
 */
class some_type_t {
public:
    some_type_t() { _numberOfConstructorCalls++; }

    some_type_t(const some_type_t &) { _numberOfConstructorCalls++; }

    some_type_t(some_type_t &&) = default;

    some_type_t &operator=(const some_type_t &) {
        _numberOfConstructorCalls++;
        return *this;
    }

    some_type_t &operator=(some_type_t &&) = default;

    static void reset() { _numberOfConstructorCalls = 0; }

    static unsigned int getNumberOfConstructorCalls() { return _numberOfConstructorCalls; }

private:
    static unsigned int _numberOfConstructorCalls;
};

/**
 * This method mimics "initialization" of memory that the
 * client of an API allocates (in any way, stack, heap, static).
 */
void do_init_work(some_type_t *);

/**
 * If the client allocates memory and hands it to the API for initialization,
 * it is usually the client's responsibility to free the memory. But first, the
 * client usually has to give the library the chance to free any resources it
 * has allocated on behalf of the client. This is our mock version of such a
 * function.
 */
void release_resources(some_type_t *ptr);

/**
 * This method is used to mimic an API that allocates memory and returns a
 * pointer. That pointer must usually be "freed" by passing it to a custom
 * "free function" (which we define below as 'free_resources'.
 */
some_type_t *create_and_initialize();

/**
 * If a library allocates memory on behalf of the client then it usually
 * provides a "free function" that will deallocate the memory and free any
 * other resources that are associated with the structure.
 *
 */
void free_resources(some_type_t *ptr);

int some_func_with_error_code(int errorCode);

}  // namespace api

struct MockCallOperator {
    bool called() const { return _called; }
    unsigned int numCalls() const { return _numCalled; }

protected:
    bool _called{false};
    unsigned int _numCalled{0};
};

struct ReleaseResourcesOperator : public MockCallOperator {
    void operator()(api::some_type_t *);
};

struct FreeResourcesOperator : public MockCallOperator {
    void operator()(api::some_type_t *ptr);
};

struct SomeFuncWithErrorCodeOperator : public MockCallOperator {
    int operator()(int errorCode);
};

class MockAPI {
public:
    const FreeResourcesOperator &freeResourcesFunc() const;

    const ReleaseResourcesOperator &releaseResourcesFunc() const;

    const SomeFuncWithErrorCodeOperator &someFuncWithErrorCode() const;

    void reset();

    void doFreeResources(api::some_type_t *ptr);

    void doReleaseResources(api::some_type_t *ptr);

    int doSomeFuncWithErrorCode(int errorCode);

    static MockAPI &instance();

private:

    FreeResourcesOperator _freeFunc;
    ReleaseResourcesOperator _releaseFunc;
    SomeFuncWithErrorCodeOperator _someFuncWithErrorCode;
};

}  // namespace mock

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

}  // namespace assertions

}  // namespace testing

}  // namespace cppc
