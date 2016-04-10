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
    CallGuard<decltype(func), int, int, bool> guard { std::move(func) };
    ASSERT_FALSE(called);
    int x = guard(8, true);
    ASSERT_TRUE(called);
    ASSERT_EQ(x, 16);
    std::cout << "Sizeof guard: " << sizeof(guard) << std::endl;
}

TEST(CallGuardTest, testCallGuardDefaultConstructor) {
    struct Functor {
        bool operator() (int x) {
            return x > 0;
        }
    };

    CallGuard<Functor, bool, int> guard{};
    ASSERT_TRUE(guard(17));
}
