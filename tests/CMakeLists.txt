include_directories(../include)

include_directories(${GTEST_INCLUDE_DIRS})                                                                                                                                

add_executable(guard_test guard_test.cpp)
target_link_libraries(guard_test ${GTEST_BOTH_LIBRARIES})
add_test(UnitTests guard_test)
