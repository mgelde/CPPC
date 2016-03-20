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

#include <memory>

#include "gtest/gtest.h"

#include "cwrap.h"

#include "test_api.h"

using namespace cwrap::guard;
using namespace cwrap::testing::mock;
using namespace cwrap::testing::assertions;

template <class T>
struct CustomDeleter {
    void operator() (T &t) const {
        release_resources(&t);
    }
};

class GuardTest : public ::testing::Test {
    public:
        void SetUp() override {
            MockAPI::instance().reset();
        }
};

TEST_F(GuardTest, testPassingPointerToAllocatedMemory) {
    {
        auto freeFunc = [this](some_type_t &ptr) {
            release_resources(&ptr);
        };
        Guard<some_type_t, decltype(freeFunc)> guard {freeFunc};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        do_init_work(&guard.get());
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
    }
    ASSERT_CALLED(MockAPI::instance().releaseResourcesFunc());
}

TEST_F(GuardTest, testInitFunctionReturningPointer) {
    {
        auto freeFunc = [this](some_type_t *&ptr) {
            free_resources(ptr);
        };
        auto guard = make_guarded<some_type_t*, decltype(freeFunc)>(freeFunc, create_and_initialize());
        ASSERT_FALSE(guard.get()->isFreed());
        ASSERT_NOT_CALLED(MockAPI::instance().freeResourcesFunc());
    }
    ASSERT_CALLED(MockAPI::instance().freeResourcesFunc());
}


TEST_F(GuardTest, testWithCustomDeleter) {
    {
        Guard<some_type_t, CustomDeleter<some_type_t>> guard {};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        do_init_work(&guard.get());
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
    }
    ASSERT_CALLED(MockAPI::instance().releaseResourcesFunc());
}

TEST_F(GuardTest, testWithUniquePointer) {
    {
        Guard<some_type_t,
            CustomDeleter<some_type_t>,
            UniquePointerStoragePolicy<some_type_t>> guard {};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        do_init_work(&guard.get());
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
    }
    ASSERT_CALLED(MockAPI::instance().releaseResourcesFunc());
}
