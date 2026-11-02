enable_testing()

add_executable(
  a51lib_test
  ResourceManagerTest.cpp
)
target_include_directories(
  a51lib_test
  BEFORE PRIVATE
  ${googletest_SOURCE_DIR}/googlemock/include
  ${googletest_SOURCE_DIR}/googletest/include
)
set_property(TARGET gtest PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "")
set_property(TARGET gtest_main PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "")
target_link_libraries(
  a51lib_test
  a51lib
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(a51lib_test)
