#   Copyright 2016,2017 Marcus Gelderie
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

if (NOT DEFINED GTEST_SOURCE_DIR)
    set (GTEST_SOURCE_DIR "/usr/src/gtest")
    message (STATUS "No GTest src location given. Defaulting to '${GTEST_SOURCE_DIR}'")
endif (NOT DEFINED GTEST_SOURCE_DIR)

if (NOT EXISTS ${GTEST_SOURCE_DIR} OR NOT IS_DIRECTORY ${GTEST_SOURCE_DIR})
    message(FATAL_ERROR "Could not find GTest sources at ${GTEST_SOURCE_DIR}")
endif (NOT EXISTS ${GTEST_SOURCE_DIR} OR NOT IS_DIRECTORY ${GTEST_SOURCE_DIR})

include(ExternalProject)
ExternalProject_Add(google-test
    URL "file://${GTEST_SOURCE_DIR}"
    INSTALL_COMMAND ""
)
find_package(Threads REQUIRED)

ExternalProject_Get_Property(google-test binary_dir source_dir)
add_library(gtest      UNKNOWN IMPORTED)
add_library(gtest_main UNKNOWN IMPORTED)
set_target_properties(gtest PROPERTIES
  IMPORTED_LOCATION ${binary_dir}/libgtest.a
)
set_target_properties(gtest_main PROPERTIES
  IMPORTED_LOCATION ${binary_dir}/libgtest_main.a
)
set(GTEST_INCLUDE_DIRS ${source_dir}/include)
set(GTEST_BOTH_LIBRARIES gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_dependencies(gtest_main google-test)
add_dependencies(gtest_main google-test)

include_directories(${GTEST_INCLUDE_DIRS})

add_library(mock_api STATIC test_api.cpp)

add_executable(guard_test guard_test.cpp)
target_link_libraries(guard_test ${GTEST_BOTH_LIBRARIES} CPPC mock_api)
add_test(GuardTests guard_test)

add_executable(checkcall_tests checkcall_tests.cpp)
target_link_libraries(checkcall_tests ${GTEST_BOTH_LIBRARIES} CPPC mock_api)
add_test(FuncWrapperTests checkcall_tests)
