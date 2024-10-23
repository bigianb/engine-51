#include <gtest/gtest.h>

#include "../a51lib/ResourceManager.h"

TEST(ResourceManagerTest, BasicOperation) {

  ResourceManager resourceManager;

  // General operation is to have a resource handle,
  // set the name and then get the pointer.

  ResourceHandleBase baseHandle;
  baseHandle.setResourceManager(&resourceManager);
  baseHandle.setName("myResourceName");

}

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}
