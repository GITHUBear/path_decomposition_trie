//
// Created by Dim Dew on 2020-07-28.
//
#include <gtest/gtest.h>
#include "balanced_parentheses_vector.h"

TEST(FIND_OPEN, FIND_OPEN_IN_WORD_1) {
    uint64_t res;
    uint64_t word = 0x0F53800000000000ULL;
    uint64_t byte_cnt = succinct::util::byte_counts(word);

    EXPECT_EQ(byte_cnt, 0x0404010000000000ULL);
    ASSERT_TRUE(succinct::find_open_in_word(word, byte_cnt, 1, res));
    EXPECT_EQ(res, 47);
}

TEST(FIND_OPEN, FIND_OPEN_IN_WORD_2) {
    uint64_t res;
    uint64_t word = 0x0F53FFFFFFFFFFFFULL;
    uint64_t byte_cnt = succinct::util::byte_counts(word);

    EXPECT_EQ(byte_cnt, 0x0404080808080808ULL);
    ASSERT_TRUE(succinct::find_open_in_word(word, byte_cnt, 1, res));
    EXPECT_EQ(res, 47);
}

TEST(FIND_OPEN, FIND_OPEN_IN_WORD_3) {
    uint64_t res;
    uint64_t word = 0x2974FFFFFFFFFFFFULL;
    uint64_t byte_cnt = succinct::util::byte_counts(word);

    EXPECT_EQ(byte_cnt, 0x0304080808080808ULL);
    ASSERT_TRUE(succinct::find_open_in_word(word, byte_cnt, 1, res));
    EXPECT_EQ(res, 45);

    ASSERT_TRUE(succinct::find_open_in_word(word, byte_cnt, 2, res));
    EXPECT_EQ(res, 44);
}

TEST(FIND_OPEN, FIND_OPEN_IN_WORD_4) {
    uint64_t res;
    uint64_t word = 0x0ULL;
    uint64_t byte_cnt = succinct::util::byte_counts(word);

    EXPECT_EQ(byte_cnt, 0x0ULL);
    ASSERT_FALSE(succinct::find_open_in_word(word, byte_cnt, 1, res));
}

GTEST_API_ int main(int argc, char ** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
