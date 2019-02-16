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

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "gtest/gtest.h"

#include "checkcall.hpp"
#include "test_api.h"

using namespace ::cppc;
using namespace ::cppc::testing::mock;
using namespace ::cppc::testing::mock::api;
using namespace ::cppc::testing::assertions;

/**
 * Auxiliary templates for testing
 */
struct WithPreCall {
    static bool called;
    static inline void preCall() { called = true; };
};
struct WithoutPreCall {
    static inline void fooCall(){};
};
bool WithPreCall::called = false;

TEST(Auxiliary, callPrecCallIfPresentSFINAE) {
    WithPreCall::called = false;
    ::cppc::_auxiliary::callPrecCallIfPresent<WithoutPreCall>();
    ASSERT_FALSE(WithPreCall::called);
    ::cppc::_auxiliary::callPrecCallIfPresent<WithPreCall>();
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

TEST_F(CheckCallTest, testStandardUsageNoexcept) {
    const auto &x = callChecked(some_func_with_error_code_noexcept, 0);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    ASSERT_EQ(x, 0);
}

TEST_F(CheckCallTest, testStandardUsageNoexceptCallCheckContext) {
    using ct = CallCheckContext<>;
    const auto &x = ct::callChecked(some_func_with_error_code_noexcept, 0);
    ASSERT_CALLED(MockAPI::instance().someFuncWithErrorCode());
    ASSERT_EQ(x, 0);
}

TEST_F(CheckCallTest, testStandardUsageCallCheckContext) {
    using ct = CallCheckContext<>;
    const auto &x = ct::callChecked(some_func_with_error_code, 0);
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

TEST_F(CheckCallTest, testCFunctionCall) {
    int called = 0;
    int x = callChecked<IsNotZeroReturnCheckPolicy>(c_api_some_func_with_error_code, 17, &called);
    ASSERT_EQ(called, 1);
    ASSERT_EQ(x, 17);
}

/**
 * Define a CustomReturnValuePolicy that modifies the return value.
 */
struct CustomErrorPolicyWithReturnValueModification {
    template <class V>
    static std::string handleError(V &&) {
        return "false";
    }
    template <class V>
    static std::string handleOk(V &&) {
        return "true";
    }
};

TEST_F(CheckCallTest, testModifyReturnValue) {
    auto rv = callChecked<IsZeroReturnCheckPolicy, CustomErrorPolicyWithReturnValueModification>(
            some_func_with_error_code, -1);
    ASSERT_EQ(rv, "false");

    rv = callChecked<IsZeroReturnCheckPolicy, CustomErrorPolicyWithReturnValueModification>(
            some_func_with_error_code, 0);
    ASSERT_EQ(rv, "true");

    static_assert(std::is_same<std::string, decltype(rv)>::value,
                  "The return type must be same as ErrorPolicies return-value-type");
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
    struct CustomReturnCheckPolicy {
        static inline bool returnValueIsOk(bool b) { return b; }
    };

    CallGuard<Functor, CustomReturnCheckPolicy> guard{};
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
    CallGuard<decltype(func), IsErrnoZeroReturnCheckPolicy> guard{std::move(func)};
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
