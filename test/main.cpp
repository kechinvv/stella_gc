#include <stdio.h>
#include <gtest/gtest.h>
extern "C" {
#include "runtime.h"
}
using namespace std;


TEST(TestGroupName, Subtest_1) {
  ASSERT_TRUE(1 == 1);
}

TEST(TestGroupName, Subtest_2) {
  ASSERT_TRUE('b' == 'b');
  cout << "continue test after failure" << endl;
}

TEST(TestGC, test1) {
  print_gc_alloc_stats();
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}