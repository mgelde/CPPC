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

#include "test_api.h"

extern "C" {

int c_api_some_func_with_error_code(int x, int *ctx) {
    if (ctx) {
        (*ctx)++;
    }
    return x;
}

}

namespace cppc {

namespace testing {

namespace mock {

namespace api {

unsigned int some_type_t::_numberOfConstructorCalls{0};

/**
 * We allocate an instance in static storage
 * so we can return a valid (raw) pointer.
 */
static some_type_t __some_type{};

/*
 * This method is a no-op because there is nothing to initialize.  We could
 * omit this from the tests and they would do/assert just as much, but they
 * would not document the intended behavior as well.
 */
void do_init_work(some_type_t *) {}

some_type_t *create_and_initialize() {
    do_init_work(&__some_type);  // in case this method ever does anything
    return &__some_type;
}

int some_func_with_error_code(int errorCode) {
    return MockAPI::instance().doSomeFuncWithErrorCode(errorCode);
}

void free_resources(some_type_t *ptr) { MockAPI::instance().doFreeResources(ptr); }

void release_resources(some_type_t *ptr) { MockAPI::instance().doReleaseResources(ptr); }

}  // namespace api

void ReleaseResourcesOperator::operator()(api::some_type_t *) {
    _called = true;
    _numCalled++;
}

void FreeResourcesOperator::operator()(api::some_type_t *ptr) {
    if (ptr != &api::__some_type) {
        throw MockException{};
    }
    release_resources(ptr);
    _called = true;
    _numCalled++;
}

int SomeFuncWithErrorCodeOperator::operator()(int errorCode) {
    _called = true;
    _numCalled++;
    return errorCode;
}

const FreeResourcesOperator &MockAPI::freeResourcesFunc() const { return _freeFunc; }

const ReleaseResourcesOperator &MockAPI::releaseResourcesFunc() const { return _releaseFunc; }

const SomeFuncWithErrorCodeOperator &MockAPI::someFuncWithErrorCode() const {
    return _someFuncWithErrorCode;
}

void MockAPI::reset() {
    _freeFunc = FreeResourcesOperator{};
    _releaseFunc = ReleaseResourcesOperator{};
    _someFuncWithErrorCode = SomeFuncWithErrorCodeOperator{};
    api::some_type_t::reset();
}

void MockAPI::doFreeResources(api::some_type_t *ptr) { _freeFunc(ptr); }

void MockAPI::doReleaseResources(api::some_type_t *ptr) { _releaseFunc(ptr); }

int MockAPI::doSomeFuncWithErrorCode(int errorCode) { return _someFuncWithErrorCode(errorCode); }

MockAPI &MockAPI::instance() {
    static std::unique_ptr<MockAPI> instance{std::make_unique<MockAPI>()};
    return *instance;
}

}  // namespace mock

}  // namespace testing

}  // namespace cppc
