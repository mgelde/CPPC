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

#include "test_api.h"

namespace cwrap {

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

}  // namespace cwrap
