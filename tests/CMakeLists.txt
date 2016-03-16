include_directories(${GTEST_INCLUDE_DIRS})    

add_executable(guard_test guard_test.cpp)
target_link_libraries(guard_test ${GTEST_BOTH_LIBRARIES})
target_link_libraries(guard_test CWrap)
add_test(GuardTests guard_test)
