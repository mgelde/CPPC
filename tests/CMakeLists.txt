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

enable_testing()
find_package(GTest QUIET)

if(NOT ${GTEST_FOUND})
	message("Did not find GTest as a binary")
	include(ExternalProject)
	ExternalProject_Add(google-test
		URL "file:///usr/src/gtest"
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
endif(NOT ${GTEST_FOUND})

include_directories(${GTEST_INCLUDE_DIRS})

add_executable(guard_test guard_test.cpp)
target_link_libraries(guard_test ${GTEST_BOTH_LIBRARIES})
target_link_libraries(guard_test CWrap)

add_test(GuardTests guard_test)
