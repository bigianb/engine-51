cmake_minimum_required(VERSION 3.16)

project(Engine51 VERSION 0.1 LANGUAGES CXX)

include(CTest)
include(FetchContent)
FetchContent_Declare(GoogleTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(a51lib)
add_subdirectory(a51lib_test)
add_subdirectory(tools)
