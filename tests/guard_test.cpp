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
