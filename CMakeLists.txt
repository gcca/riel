cmake_minimum_required(VERSION 3.11)

project(rem)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER clang++-8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
-g3 \
-O0 \
-std=gnu++17 \
-Wall \
-Werror \
-Wextra \
-pedantic-errors \
-Wconversion \
-Wunreachable-code \
-pedantic-errors \
-Wold-style-cast \
-Weverything \
-Wno-c++98-compat \
-Wno-c++98-compat-pedantic \
-Wshadow \
-Wfloat-equal \
-Weffc++ \
-Wno-global-constructors"
)

find_package(Threads)

include(GoogleTest)
include(ExternalProject)

ExternalProject_Add(googletest_ep
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=RELEASE
    -DCMAKE_INSTALL_PREFIX=prefix
  EXCLUDE_FROM_ALL TRUE
  GIT_REPOSITORY git@github.com:google/googletest.git
  GIT_TAG release-1.8.1
  UPDATE_COMMAND "")
ExternalProject_Get_property(googletest_ep BINARY_DIR)
set(GTEST_ROOT ${BINARY_DIR}/prefix)

set(GTEST_FOUND ON)

file(MAKE_DIRECTORY ${GTEST_ROOT}/include)
file(MAKE_DIRECTORY ${GTEST_ROOT}/lib)

add_library(GTest::GTest INTERFACE IMPORTED)
add_dependencies(GTest::GTest googletest_ep)
target_include_directories(GTest::GTest INTERFACE ${GTEST_ROOT}/include)
target_link_libraries(GTest::GTest
    INTERFACE ${GTEST_ROOT}/lib/libgtest.a Threads::Threads)

add_library(GTest::Main INTERFACE IMPORTED)
add_dependencies(GTest::Main GTest::GTest)
target_link_libraries(GTest::Main INTERFACE ${GTEST_ROOT}/lib/libgtest_main.a)

enable_testing()

add_library(riel
  src/riel/riel.cc
)
target_include_directories(riel PUBLIC src)

add_executable(riel-test
  EXCLUDE_FROM_ALL
  src/riel/riel-test.cc
)
target_link_libraries(riel-test riel GTest::GTest GTest::Main)
gtest_add_tests(TARGET riel-test)

add_custom_target(tests
  DEPENDS
  riel-test
)
