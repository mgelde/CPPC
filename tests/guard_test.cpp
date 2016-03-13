#include <memory>

#include "gtest/gtest.h"

#include "cwrap.h"

struct SomeClass {
    int x = 17;
    int *blarg;
};

using namespace cwrap::guard;

static int staticData = 1773;

class GuardTest : public testing::Test {
    public:
        void SetUp() override;
        void TearDown() override;
    protected:
        std::unique_ptr<SomeClass> _ptr;
};

void GuardTest::SetUp() {
    _ptr = std::make_unique<SomeClass>();
    _ptr->blarg = &staticData;
}

void GuardTest::TearDown() {
    _ptr.reset();
}

TEST_F(GuardTest, testFreeFunction) {
     {
         Guard<SomeClass*> guard (_ptr.get(), [](SomeClass *p) {
            if (nullptr != p && nullptr != p->blarg) {
                p->blarg = &p->x;
            }
        });
        ASSERT_EQ(guard.get(), _ptr.get());
        ASSERT_NE(nullptr, guard.get()->blarg);
        ASSERT_NE(guard.get()->blarg, &guard.get()->x);
     }
     ASSERT_EQ(_ptr->blarg, &_ptr->x);
}
