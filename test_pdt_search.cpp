//
// Created by Dim Dew on 2020-10-09.
//
#include <gtest/gtest.h>
#include "path_decomposed_trie.h"

std::vector<uint8_t> string_to_bytes(std::string s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

template <typename TrieBuilder>
void append_to_trie(
        succinct::trie::compacted_trie_builder<TrieBuilder>& builder,
        std::string s) {
    auto s1 = string_to_bytes(s);
    builder.append(s1);
}

std::string get_label(const std::vector<uint16_t>& labels) {
    std::string label;
    for (auto byte : labels) {
        if (byte >> 8 == 1) {
            label += std::to_string(uint8_t(byte));
        } else if (byte >> 8 == 2) {
            label += '#';
        } else {
            label += (char(byte) ? char(byte) : '$');
        }
    }
    return label;
}

std::string get_bp_str(const succinct::BpVector& bp) {
    size_t bp_idx = 0;
    std::string bp_str;
    while (bp_idx < bp.size()) {
        if (bp[bp_idx]) {
            bp_str += "(";
        } else {
            bp_str += ")";
        }
        bp_idx++;
    }
    return bp_str;
}

std::string get_branch_str(const std::vector<uint8_t>& branches) {
    std::string branch_str;
    for (auto byte : branches) {
        branch_str += (char)byte;
    }
    return branch_str;
}

TEST(PDT_TEST, CREATE_1) {
    succinct::DefaultTreeBuilder<true> pdt_builder;
    succinct::trie::compacted_trie_builder
            <succinct::DefaultTreeBuilder<true>>
            trieBuilder(pdt_builder);
    append_to_trie(trieBuilder, "three");
    append_to_trie(trieBuilder, "trial");
    append_to_trie(trieBuilder, "triangle");
    append_to_trie(trieBuilder, "triangular");
    append_to_trie(trieBuilder, "triangulate");
    append_to_trie(trieBuilder, "triangulaus");
    append_to_trie(trieBuilder, "trie");
    append_to_trie(trieBuilder, "triple");
    append_to_trie(trieBuilder, "triply");
    trieBuilder.finish();

//    auto root = trieBuilder.get_root();
//    succinct::BpVector bpVec(&root->m_bp, false, true);
//    size_t bp_idx = 0;
//    std::string bp_str;
//    while (bp_idx < bpVec.size()) {
//        if (bpVec[bp_idx]) {
//            bp_str += "(";
//        } else {
//            bp_str += ")";
//        }
//        bp_idx++;
//    }
//    printf("%s\n", bp_str.c_str());

    succinct::trie::DefaultPathDecomposedTrie<true> pdt(trieBuilder);
    EXPECT_EQ(get_label(pdt.get_labels()), "t0hree#i1a0l#g0le#la1r#e#s##l0e##");
    EXPECT_EQ(get_branch_str(pdt.get_branches()), "rpenuuty");
    EXPECT_EQ(get_bp_str(pdt.get_bp()), "(()((()()(())))())");
//    EXPECT_EQ(get_bp_str(pdt.get_bp()), "");
    for (auto& pos : pdt.word_positions) {
        printf("%lu ", pos);
    }
    printf("\n");


}

GTEST_API_ int main(int argc, char ** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}