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

/**
 * Tests for the auxiliary templates
 */
struct WithPreCall {
    static bool called;
    static inline void preCall() { called = true; };
};
struct WithoutPreCall {
    static inline void fooCall(){};
};
bool WithPreCall::called = false;

TEST(Auxiliary, testSFINAEDetectsMethod) {
    ASSERT_TRUE(::cwrap::_auxiliary::HasPreCall<WithPreCall>::value);
    ASSERT_FALSE(::cwrap::_auxiliary::HasPreCall<WithoutPreCall>::value);
}

TEST(Auxiliary, testCallIfTemplate) {
    WithPreCall::called = false;
    ::cwrap::_auxiliary::CallIf<false, WithPreCall>::call();
    ASSERT_FALSE(WithPreCall::called);
    ::cwrap::_auxiliary::CallIf<true, WithPreCall>::call();
    ASSERT_TRUE(WithPreCall::called);
}

/**
 * Tests for the CHECK_CALL function template.
 */
class CheckCallTest : public ::testing::Test {
public:
    void SetUp() override {
        MockAPI::instance().reset();
        ASSERT_NOT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    }
};

TEST_F(CheckCallTest, testStandardUsage) {
    const auto &x = callChecked(some_func_with_error_code, 0);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    ASSERT_EQ(x, 0);
}

TEST_F(CheckCallTest, testWithNonDefaultReturnCheckPolicy) {
    const auto &x = callChecked<IsNotZeroReturnCheckPolicy>(some_func_with_error_code, 1);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    ASSERT_EQ(x, 1);
    ASSERT_THROW(callChecked<IsNotZeroReturnCheckPolicy>(some_func_with_error_code, 0),
                 std::runtime_error);
}

TEST_F(CheckCallTest, testWithNonDefaultErrorPolicy) {
    const auto &x = callChecked<IsNotZeroReturnCheckPolicy, ErrorCodeErrorPolicy>(
            some_func_with_error_code, 1);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    ASSERT_EQ(x, 1);
    // we need a redundant pair of parenthesis to make the C pre-processor swallow the template
    // arguments
    ASSERT_THROW((callChecked<IsZeroReturnCheckPolicy, ErrorCodeErrorPolicy>(
                         some_func_with_error_code, -1)),
                 std::runtime_error);
}

/**
 * Tests for the CallGuard class.
 */
class CallGuardTest : public ::testing::Test {
public:
    void SetUp() override {
        MockAPI::instance().reset();
        _called = false;
        _func = [this](int x) {
            _called = true;
            return x;
        };
    }

protected:
    bool _called{false};
    std::function<int(int)> _func;
};

TEST_F(CallGuardTest, testCallGuardClassCallCorrectly) {
    CallGuard<decltype(_func), IsNotNegativeReturnCheckPolicy> guard{std::move(_func)};
    ASSERT_FALSE(_called);
    const auto x = guard(8);
    ASSERT_TRUE(_called);
    ASSERT_EQ(x, 8);
}

TEST_F(CallGuardTest, functionPointerTest) {
    CallGuard<decltype(some_func_with_error_code), IsNotNegativeReturnCheckPolicy> guard{
            some_func_with_error_code};
    const auto x = guard(17);
    ASSERT_EQ(x, 17);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
}

TEST_F(CallGuardTest, testCallGuardDefaultConstructor) {
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

TEST_F(CallGuardTest, testIsZeroReturnCheckPolicy) {
    // IsZeroReturnCheckPolicy should be default, so don't specify it.
    CallGuard<decltype(_func)> guard{std::move(_func)};
    ASSERT_THROW(guard(1), std::runtime_error);
    ASSERT_THROW(guard(-1), std::runtime_error);
    ASSERT_NO_THROW(guard(0));
}

TEST_F(CallGuardTest, testIsNotNegativeCheckPolicy) {
    CallGuard<decltype(_func), IsNotNegativeReturnCheckPolicy> guard{std::move(_func)};
    ASSERT_NO_THROW(guard(1));
    ASSERT_NO_THROW(guard(0));
    ASSERT_THROW(guard(-1), std::runtime_error);
}

TEST_F(CallGuardTest, testIsNotZeroCheckPolicy) {
    CallGuard<decltype(_func), IsNotZeroReturnCheckPolicy> guard{std::move(_func)};
    ASSERT_NO_THROW(guard(1));
    ASSERT_NO_THROW(guard(-1));
    ASSERT_THROW(guard(0), std::runtime_error);
}

TEST_F(CallGuardTest, testIsNotNullCheckPolicy) {
    auto func = [](const int *ptr) { return ptr; };
    const int x = 17;
    CallGuard<decltype(func), IsNotNullptrReturnCheckPolicy> guard{std::move(func)};
    ASSERT_NO_THROW(guard(&x));
    ASSERT_THROW(guard(nullptr), std::runtime_error);
}

TEST_F(CallGuardTest, testIsErrnoZeroReturnCheckPolicy) {
    bool wasErrnoZero{false};
    auto func = [&wasErrnoZero](int i) {
        wasErrnoZero = (errno == 0);
        errno = i;
        return wasErrnoZero;
    };
    CallGuard<decltype(func), IsErrnoZeroReturnCheckPolicy > guard{std::move(func)};
    errno = 17;  // set errno to verify that IsErrnoZeroReturnCheckPolicy resets this to zero
    ASSERT_NO_THROW(guard(0));
    ASSERT_TRUE(wasErrnoZero);
    ASSERT_THROW(guard(1), std::runtime_error);
}

TEST_F(CallGuardTest, defaultErrorPolicyTest) {
    CallGuard<decltype(_func), IsNotNegativeReturnCheckPolicy> guard{std::move(_func)};
    try {
        guard(-1337);
    } catch (const std::runtime_error &e) {
        ASSERT_NE(std::string::npos, std::string(e.what()).find("-1337"));
        return;
    }
    FAIL() << "Execution should not reach this line";
}

TEST_F(CallGuardTest, testErrnoErrorPolicy) {
    CallGuard<decltype(_func), IsNotNegativeReturnCheckPolicy, ErrnoErrorPolicy> guard{
            std::move(_func)};
    errno = EINVAL;
    try {
        guard(-1337);
    } catch (const std::runtime_error &e) {
        ASSERT_EQ(std::string(e.what()), std::string(std::strerror(EINVAL)));
        return;
    }
    FAIL() << "Execution should not reach this line";
}

TEST_F(CallGuardTest, testErrorCodeErrorPolicy) {
    CallGuard<decltype(_func), IsNotNegativeReturnCheckPolicy, ErrorCodeErrorPolicy> guard{
            std::move(_func)};
    try {
        guard(-EINVAL);
    } catch (const std::runtime_error &e) {
        ASSERT_EQ(std::string(e.what()), std::string(std::strerror(EINVAL)));
        return;
    }
    FAIL() << "Execution should not reach this line";
}

TEST_F(CallGuardTest, testReportReturnValueErrorPolicy) {
    CallGuard<decltype(_func), IsNotNegativeReturnCheckPolicy, ReportReturnValueErrorPolicy> guard{
            std::move(_func)};
    try {
        guard(-1337);
    } catch (const std::runtime_error &e) {
        ASSERT_EQ(std::string(e.what()), std::string("Return value indicated error: -1337"));
        return;
    }
    FAIL() << "Execution should not reach this line";
}
