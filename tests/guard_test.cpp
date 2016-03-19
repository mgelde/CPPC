#include <memory>

#include "gtest/gtest.h"

#include "cwrap.h"

#include "test_api.h"

using namespace cwrap::guard;

class GuardTest : public testing::Test {
    public:
        void SetUp() override;
        void TearDown() override;

        static bool _called;
};

bool GuardTest::_called { false };

void GuardTest::SetUp() {
    _called = false;
}

template <class T>
struct CustomDeleter {
    void operator() (T &t) const {
        GuardTest::_called = true;
        (void) t;
    }
};

void GuardTest::TearDown() {
}

TEST_F(GuardTest, testPassingPointerToAllocatedMemory) {
    {
        auto freeFunc = [this](some_type_t &ptr) {
            ptr.x = 2222;
            ptr.data = nullptr;
            some_free_func(&ptr);
            _called = true;
        };
        ASSERT_FALSE(_called);
        Guard<some_type_t, decltype(freeFunc)> guard {freeFunc};
        ASSERT_FALSE(_called);
        do_init_work(&guard.get());
        ASSERT_FALSE(_called);
    }
    ASSERT_TRUE(_called);
}

TEST_F(GuardTest, testInitFunctionReturningPointer) {
    {
        auto freeFunc = [this](some_type_t *&ptr) {
            _called = true;
            some_free_func(ptr);
        };
        auto guard = make_guarded<some_type_t*, decltype(freeFunc)>(freeFunc, do_init_work_return());
        ASSERT_FALSE(_called);
    }
    ASSERT_TRUE(_called);
}


TEST_F(GuardTest, testWithCustomDeleter) {
    {
        ASSERT_FALSE(_called);
        Guard<some_type_t, CustomDeleter<some_type_t>> guard {};
        do_init_work(&guard.get());
        ASSERT_FALSE(_called);
    }
    ASSERT_TRUE(_called);
}

