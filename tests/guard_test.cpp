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

#include <memory>

#include "gtest/gtest.h"

#include "cwrap.hpp"

#include "test_api.h"

using namespace cwrap::guard;
using namespace cwrap::testing::mock;
using namespace cwrap::testing::mock::api;
using namespace cwrap::testing::assertions;

template <class T = DefaultFreePolicy<some_type_t>, class S = ByValueStoragePolicy<some_type_t>>
using GuardT = Guard<some_type_t, T, S>;

template <class T>
struct CustomDeleter {
    static unsigned int numberOfConstructorCalls;

    CustomDeleter() { numberOfConstructorCalls++; }

    CustomDeleter(const CustomDeleter &) { numberOfConstructorCalls++; }

    CustomDeleter(CustomDeleter &&) = default;

    CustomDeleter &operator=(const CustomDeleter &&) { numberOfConstructorCalls++; }

    CustomDeleter &operator=(CustomDeleter &&) = default;

    void operator()(T &t) const { release_resources(&t); }
};

template <class T>
unsigned int CustomDeleter<T>::numberOfConstructorCalls{0};

using CustomDeleterT = CustomDeleter<some_type_t>;

class GuardFreeFuncTest : public ::testing::Test {
public:
    void SetUp() override { MockAPI::instance().reset(); }
};

TEST_F(GuardFreeFuncTest, testPassingPointerToAllocatedMemory) {
    {
        auto freeFunc = [](some_type_t &ptr) { release_resources(&ptr); };
        GuardT<> guard{std::move(freeFunc)};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        do_init_work(&guard.get());
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
    }
    ASSERT_CALLED(MockAPI::instance().releaseResourcesFunc());
}

TEST_F(GuardFreeFuncTest, testInitFunctionReturningPointer) {
    {
        auto freeFunc = [](some_type_t *ptr) { free_resources(ptr); };
        auto guard = Guard<some_type_t *>{freeFunc, create_and_initialize()};
        ASSERT_NOT_CALLED(MockAPI::instance().freeResourcesFunc());
    }
    ASSERT_CALLED(MockAPI::instance().freeResourcesFunc());
}

TEST_F(GuardFreeFuncTest, testWithCustomDeleter) {
    {
        GuardT<CustomDeleterT> guard{};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        do_init_work(&guard.get());
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
    }
    ASSERT_CALLED(MockAPI::instance().releaseResourcesFunc());
}

TEST_F(GuardFreeFuncTest, testWithUniquePointer) {
    {
        GuardT<CustomDeleterT, UniquePointerStoragePolicy<some_type_t>> guard{};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        do_init_work(&guard.get());
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        static_assert(std::is_const<std::remove_reference_t<decltype(
                              const_cast<const decltype(guard) &>(guard).get())>>::value,
                      "Should return a const ref");
    }
    ASSERT_CALLED(MockAPI::instance().releaseResourcesFunc());
}

class GuardMemoryMngmtTest : public ::testing::Test {
public:
    void SetUp() override {
        CustomDeleterT::numberOfConstructorCalls = 0;
        MockAPI::instance().reset();
    }
};

/**
 * Make sure that we do not copy the object that is to be guarded in
 * needlessly.
 */
TEST_F(GuardMemoryMngmtTest, testGuardedValueIsPassedByReferece) {
    ASSERT_EQ(some_type_t::getNumberOfConstructorCalls(), 0);
    some_type_t someObject {};
    ASSERT_EQ(some_type_t::getNumberOfConstructorCalls(), 1);
    GuardT<CustomDeleterT> guard { someObject };
    //now we expect an additional call for the copy-construction of the
    // value stored inside the Guard
    ASSERT_EQ(some_type_t::getNumberOfConstructorCalls(), 2);

    //let's make sure the same overhead is incurred by the other overloads...
    CustomDeleterT freeFunc {};
    //...with copy-constructing the deleter
    GuardT<CustomDeleterT> anotherGuard { freeFunc, someObject };
    ASSERT_EQ(some_type_t::getNumberOfConstructorCalls(), 3);
    //...with move-constructing the deleter
    GuardT<CustomDeleterT> yetAnotherGuard { std::move(freeFunc), someObject };
    ASSERT_EQ(some_type_t::getNumberOfConstructorCalls(), 4);
}

// test that the overload taking only a deleter forwards correctly
TEST_F(GuardMemoryMngmtTest, testPerfectForwarding) {
    CustomDeleterT deleter{};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 1);
    GuardT<CustomDeleterT> guard{deleter};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 2);
    GuardT<CustomDeleterT> anotherGuard{std::move(deleter)};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 2);
    GuardT<CustomDeleterT> yetAntotherGuard{};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 3);
}

// test that the overload taking a guarded type forwards correctly
TEST_F(GuardMemoryMngmtTest, testPerfectForwardingWithTwoArguments) {
    CustomDeleterT deleter{};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 1);
    GuardT<CustomDeleterT> guard{deleter, some_type_t{}};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 2);
    GuardT<CustomDeleterT> anotherGuard{std::move(deleter), some_type_t{}};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 2);
}

// test that the right overloads are called if the deleter is a reference-type
TEST_F(GuardMemoryMngmtTest, testDeleterAsReference) {
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 0);
    CustomDeleterT deleter{};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 1);
    GuardT<CustomDeleterT &> guard{deleter};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 1);
    GuardT<CustomDeleterT &> anotherGuard{deleter, some_type_t{}};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 1);
}

TEST_F(GuardMemoryMngmtTest, testDeleterAsConstReference) {
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 0);
    const CustomDeleterT deleter{};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 1);
    GuardT<const CustomDeleterT &> guard{deleter};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 1);
    GuardT<const CustomDeleterT &> anotherGuard{deleter, some_type_t{}};
    ASSERT_EQ(CustomDeleterT::numberOfConstructorCalls, 1);
}

TEST_F(GuardMemoryMngmtTest, testMoveConstruction) {
    {
        GuardT<CustomDeleterT> guard{};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        GuardT<CustomDeleterT> another{std::move(guard)};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
    }
    ASSERT_NUM_CALLED(MockAPI::instance().releaseResourcesFunc(), 1);
}

TEST_F(GuardMemoryMngmtTest, testMoveAssignment) {
    {
        GuardT<CustomDeleterT> guard{};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        GuardT<CustomDeleterT> another{std::move(guard)};
        ASSERT_NOT_CALLED(MockAPI::instance().releaseResourcesFunc());
        guard = GuardT<CustomDeleterT>{};
    }
    ASSERT_NUM_CALLED(MockAPI::instance().releaseResourcesFunc(), 2);
}

/*
 * Static tests.
 *
 */

class TypeWithouDefaultConstructor {
    TypeWithouDefaultConstructor(int) {}
};

using NonDefaultConstructibleGuard = GuardT<TypeWithouDefaultConstructor>;

static_assert(!std::is_default_constructible<NonDefaultConstructibleGuard>::value,
              "If deleter has no default constructor, should not be "
              "default-construtible");

static_assert(!std::is_default_constructible<GuardT<CustomDeleterT &>>::value,
              "If deleter is ref-type, should not be default-construtible");

static_assert(std::is_default_constructible<Guard<some_type_t>>::value,
              "Default deleter is default constructible");

static_assert(
        std::is_const<
                std::remove_reference_t<decltype(std::declval<const Guard<int>>().get())>>::value,
        "Policy respects const correctness.");

auto __deleterLambdaPrototype = [](some_type_t &) {
};  // lambdas cannot be declared in unevaluated contexts

static_assert(std::is_constructible<GuardT<std::function<void(some_type_t &)>>,
                                    decltype(__deleterLambdaPrototype)>::value,
              "Constructor taking deleter policy should obey natural conversions");

static_assert(!std::is_copy_assignable<GuardT<CustomDeleterT>>::value,
              "Copy assignment should be disabled");

static_assert(!std::is_copy_constructible<GuardT<CustomDeleterT>>::value,
              "Copy construction should be disabled");

