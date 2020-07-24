//
// Created by Dim Dew on 2020-07-23.
//

#ifndef PATH_DECOMPOSITION_TRIE_BALANCED_PARENTHESES_VECTOR_H
#define PATH_DECOMPOSITION_TRIE_BALANCED_PARENTHESES_VECTOR_H

#include "rank_select_bit_vector.h"

namespace succinct {
    //
    // In BpVector, 64bits are organized logically in the following format:
    //
    //                                              +---- block_min_excess(calculated at the beginning of block)
    //                                              |
    //                                              V
    //           +-----+-----+-----+-----+          +-----+-----+-----+-----+
    //           |     |     |     |     |    ...   |     |     |     |     |   ...
    //           +-----+-----+-----+-----+          +-----+-----+-----+-----+
    //           |<-   bp_block_size   ->|          |<-   bp_block_size   ->|
    //           |<-                  superblock_size                     ->|
    //
    // `m_block_excess_min_` and `m_superblock_excess_min_` maintain minimum excess
    // (the number of mismatched "(") in different logical level:
    //
    //                                 4superblock
    //                                 excess
    //                                 min
    //                          /                        \
    //                         /                          \
    //                        /                            \
    //                       /                              \
    //
    //                2superblock                         2superblock
    //                excess                              excess
    //                min                                 min
    //             /                \                 /                 \
    //            /                  \               /                   \
    //         1superblock      1superblock       1superblock       1superblock
    //         excess           excess            excess            excess
    //         min              min               min               min
    //     +--------------+  +--------------+  +--------------+  +--------------+
    //     |superblock    |  |superblock    |  |superblock    |  |superblock    |
    //     | +---+   +---+|  | +---+   +---+|  | +---+   +---+|  | +---+   +---+|
    //     | |blk|...|blk||  | |blk|...|blk||  | |blk|...|blk||  | |blk|...|blk||  ...
    //     | +---+   +---+|  | +---+   +---+|  | +---+   +---+|  | +---+   +---+|
    //     +--------------+  +--------------+  +--------------+  +--------------+
    //
    class BpVector : public RsBitVector {
    public:
        BpVector() : RsBitVector() {}

        BpVector(const std::vector<bool>& bools,
                 bool with_select_hints = false,
                 bool with_select0_hints = false)
                 : RsBitVector(bools, with_select_hints, with_select0_hints) {

        }

        void swap(BpVector& other) {
            RsBitVector::swap(other);
            std::swap(m_internal_nodes_, other.m_internal_nodes_);
            m_block_excess_min_.swap(other.m_block_excess_min_);
            m_superblock_excess_min_.swap(other.m_superblock_excess_min_);
        }

        typedef int32_t excess_t;

    protected:
        static const size_t bp_block_size = 4; // to increase confusion, bp block_size is not necessarily rs_bit_vector block_size
        static const size_t superblock_size = 32; // number of blocks in superblock

        typedef int16_t block_min_excess_t;

        //
        // `block`: block index
        inline excess_t get_block_excess(uint64_t block) const {
            uint64_t word_idx = block * bp_block_size;
            uint64_t block_pos = word_idx * 64;
            excess_t excess = static_cast<excess_t>(2 * word_rank(word_idx) - block_pos);
            assert(excess >= 0);
            return excess;
        }

        void build_min_tree();

        // In fact, `m_internal_nodes` is not the number of internal nodes
        // in `m_superblock_excess_min_`. Actually it's the offset of leaf nodes
        // in `m_superblock_excess_min_`.
        uint64_t m_internal_nodes_;
        mappable_vector<block_min_excess_t> m_block_excess_min_;
        // Exactly, `m_superblock_excess_min_` is a binary tree stored in vector.
        mappable_vector<excess_t> m_superblock_excess_min_;

    };
}

#endif //PATH_DECOMPOSITION_TRIE_BALANCED_PARENTHESES_VECTOR_H
