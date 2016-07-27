/*
 *  CWrap - A library to use C-code in C++.
 *  Copyright (C) 2016  Marcus Gelderie
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "gtest/gtest.h"

#include "checkcall.hpp"
#include "test_api.h"

using namespace ::cwrap;
using namespace ::cwrap::testing::mock;
using namespace ::cwrap::testing::mock::api;
using namespace ::cwrap::testing::assertions;

TEST(CheckCallTest, testStadardUsage) {
    MockAPI::instance().reset();
    ASSERT_NOT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    const auto x = CALL_CHECKED(some_func_with_error_code, 0);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    ASSERT_EQ(x, 0);
}

TEST(CheckCallTest, testIsNotZeroReturnCheckPolicy) {
    MockAPI::instance().reset();
    ASSERT_NOT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    const auto x = CALL_CHECKED<IsNotZeroReturnCheckPolicy>(some_func_with_error_code, 1);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    ASSERT_EQ(x, 1);
}

TEST(CallGuardTest, testCallGuardClassCallCorrectly) {
    bool called{false};
    auto func = [&called](int x, bool y) {
        called = true;
        return y ? 2 * x : x;
    };
    CallGuard<decltype(func), IsNotNegativeReturnCheckPolicy> guard{std::move(func)};
    ASSERT_FALSE(called);
    const auto x = guard(8, true);
    ASSERT_TRUE(called);
    ASSERT_EQ(x, 16);
}

TEST(CallGuardTest, functionPointerTest) {
    MockAPI::instance().reset();
    CallGuard<decltype(some_func_with_error_code), IsNotNegativeReturnCheckPolicy> guard{
            some_func_with_error_code};
    const auto x = guard(17);
    ASSERT_EQ(x, 17);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
}

TEST(CallGuardTest, testCallGuardDefaultConstructor) {
    // functor has a default constructor
    struct Functor {
        bool operator()(int x) { return x > 0; }
    };
    struct CustomReturnPolicy {
        static inline bool returnValueIsOk(bool b) { return b; }
    };

    CallGuard<Functor, CustomReturnPolicy> guard{};
    ASSERT_TRUE(guard(17));
    static_assert(std::is_same<bool, decltype(guard(17))>::value,
                  "Return type should be identical to functor");
}

TEST(CallGuardTest, testIsZeroReturnCheckPolicy) {
    auto lambda = [](int x) { return x; };
    // IsZeroReturnCheckPolicy should be default, so don't specify it.
    CallGuard<decltype(lambda)> guard{std::move(lambda)};
    ASSERT_THROW(guard(1), std::runtime_error);
    ASSERT_THROW(guard(-1), std::runtime_error);
    ASSERT_NO_THROW(guard(0));
}

TEST(CallGuardTest, testIsNotNegativeCheckPolicy) {
    auto lambda = [](int x) { return x; };
    CallGuard<decltype(lambda), IsNotNegativeReturnCheckPolicy> guard{std::move(lambda)};
    ASSERT_NO_THROW(guard(1));
    ASSERT_NO_THROW(guard(0));
    ASSERT_THROW(guard(-1), std::runtime_error);
}

TEST(CallGuardTest, defaultErrorPolicyTest) {
    auto lambda = [](int x) { return x; };
    CallGuard<decltype(lambda), IsNotNegativeReturnCheckPolicy> guard{std::move(lambda)};
    try {
        guard(-1337);
    } catch (const std::runtime_error &e) {
        ASSERT_NE(std::string::npos, std::string(e.what()).find("-1337"));
        return;
    }
    FAIL() << "Execution should not reach this line";
}

TEST(CallGuardTest, testErrnoErrorPolicy) {
    auto lambda = [](int x) { return x; };
    CallGuard<decltype(lambda), IsNotNegativeReturnCheckPolicy, ErrnoErrorPolicy> guard{
            std::move(lambda)};
    errno = EINVAL;
    try {
        guard(-1337);
    } catch (const std::runtime_error &e) {
        ASSERT_EQ(std::string(e.what()), std::string(std::strerror(EINVAL)));
        return;
    }
    FAIL() << "Execution should not reach this line";
}

TEST(CallGuardTest, testErrorCodeErrorPolicy) {
    auto lambda = [](int x) { return x; };
    CallGuard<decltype(lambda), IsNotNegativeReturnCheckPolicy, ErrorCodeErrorPolicy> guard{
            std::move(lambda)};
    try {
        guard(-EINVAL);
    } catch (const std::runtime_error &e) {
        ASSERT_EQ(std::string(e.what()), std::string(std::strerror(EINVAL)));
        return;
    }
    FAIL() << "Execution should not reach this line";
}
