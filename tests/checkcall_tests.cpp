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

#include <iostream>
#include <stdexcept>

#include "gtest/gtest.h"

#include "test_api.h"
#include "checkcall.h"

using namespace cwrap::error;

TEST(CallGuardTest, testCallGuardClassCallCorrectly) {
    bool called { false };
    auto func = [&called] (int x, bool y) {
        called = true;
        return y ? 2*x : x;
    };
    CallGuard<decltype(func),
        int,
        IsNotNegativeReturnCheckPolicy<int>> guard { std::move(func) };
    ASSERT_FALSE(called);
    int x = guard(8, true);
    ASSERT_TRUE(called);
    ASSERT_EQ(x, 16);
}

TEST(CallGuardTest, testCallGuardDefaultConstructor) {
    struct Functor {
        bool operator() (int x) {
            return x > 0;
        }
    };
    struct CustomReturnPolicy {
        static inline bool returnValueIsOk(bool b) {
            return b;
        }
    };

    CallGuard<Functor, bool, CustomReturnPolicy> guard{};
    ASSERT_TRUE(guard(17));
}

TEST(CallGuardTest, testIsZeroReturnCheckPolicy) {
    auto lambda = [](int x) { return x; };
    //IsZeroReturnCheckPolicy should be default, so don't specify it.
    CallGuard<decltype(lambda), int> guard { std::move(lambda) };
    ASSERT_THROW(guard(1), std::runtime_error);
    ASSERT_THROW(guard(-1), std::runtime_error);
    ASSERT_NO_THROW(guard(0));
}

TEST(CallGuardTest, testIsNotNegativeCheckPolicy) {
    auto lambda = [](int x) { return x; };
    CallGuard<decltype(lambda),
        int,
        IsNotNegativeReturnCheckPolicy<int>> guard { std::move(lambda) };
    ASSERT_NO_THROW(guard(1));
    ASSERT_NO_THROW(guard(0));
    ASSERT_THROW(guard(-1), std::runtime_error);
}
