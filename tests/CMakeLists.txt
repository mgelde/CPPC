#
#   CWrap - A library to use C-code in C++.
#   Copyright (C) 2016  Marcus Gelderie
#
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
#   USA
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

add_executable(guard_test guard_test.cpp)
target_link_libraries(guard_test ${GTEST_BOTH_LIBRARIES})
target_link_libraries(guard_test CWrap)
add_test(GuardTests guard_test)

add_executable(checkcall_tests checkcall_tests.cpp)
target_link_libraries(checkcall_tests ${GTEST_BOTH_LIBRARIES})
target_link_libraries(checkcall_tests CWrap)
add_test(FuncWrapperTests checkcall_tests)
