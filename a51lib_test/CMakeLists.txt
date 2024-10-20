enable_testing()

add_executable(
  a51lib_test
  ResourceManagerTest.cpp
)
target_link_libraries(
  a51lib_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(a51lib_test)
