#pragma once

struct some_type_t {
    int x;
    int *data;
};

constexpr int TEST_ERR_INVAL = 1;

void do_init_work(some_type_t *ptr) {
    if (nullptr == ptr) {
        return;
    }
    ptr->x = 17;
    ptr->data = new int;
    *ptr->data = 19;
}

some_type_t *do_init_work_return() {
    return new some_type_t;
}

void some_free_func(some_type_t *ptr) {
    if (ptr) {
        if (ptr->data) {
            delete ptr;
            ptr->data = nullptr;
        }
        ptr->x = 0;
    }
}
