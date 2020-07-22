//
// Created by Dim Dew on 2020-07-21.
//
#include <gtest/gtest.h>
#include <iostream>
#include "bit_vector.h"

int add(int a, int b) {
    return a + b;
}

TEST(test, c1) {
EXPECT_EQ(3, add(1, 2));
}

GTEST_API_ int main(int argc, char ** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
