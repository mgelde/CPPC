cmake_minimum_required (VERSION 3.0.2)
project (CWrap)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")


enable_testing()
find_package(GTest REQUIRED)

add_library(CWrap INTERFACE)
target_include_directories(CWrap INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include/cwrapinclud>
)

add_subdirectory(tests)
