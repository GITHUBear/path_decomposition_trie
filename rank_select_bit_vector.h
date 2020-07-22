//
// Created by Dim Dew on 2020-07-22.
//

#ifndef PATH_DECOMPOSITION_TRIE_RANK_SELECT_BIT_VECTOR_H
#define PATH_DECOMPOSITION_TRIE_RANK_SELECT_BIT_VECTOR_H

#include "bit_vector.h"

namespace succinct {

    // In BitVector, bits are combined in the following format:
    //
    //         64bit  64bit        64bit  64bit
    //       +------+------+     +------+------+
    //       |      |      | ... |      |      |
    //       +------+------+     +------+------+
    //
    // While in RsBitVector, 64bits are organized logically in the following format:
    //
    //                 +-----+-----+-----+
    //       block 0   |     |     |     | ...  length: block_size
    //                 +-----+-----+-----+
    //                 A                     A
    //                 |                     |
    //                 next_rank             sub_rank
    //                 (calculated at        (calculated at
    //                 block head)           block tail)
    //
    //       block 1    ......
    //
    //         ...
    //
    //   dummy tail block
    //          A
    //          |
    // at last, a dummy tail block is used to save the number of one-bits in `next_rank`
    //
    class RsBitVector : public BitVector {
    public:
        RsBitVector()
            : BitVector()
        {}

        RsBitVector(const std::vector<bool>& bools,
                    bool with_select_hints = false,
                    bool with_select0_hints = false)
                    : BitVector(bools) {

        }

        void swap(RsBitVector& other) {
            BitVector::swap(other);
            m_block_rank_pairs_.swap(other.m_block_rank_pairs_);
            m_select_hints_.swap(other.m_select_hints_);
            m_select0_hints_.swap(other.m_select0_hints_);
        }

        inline uint64_t num_ones() const {
            return *(m_block_rank_pairs_.end() - 2);
        }

        inline uint64_t num_zeros() const {
            return size() - num_ones();
        }

    protected:
        inline uint64_t num_blocks() const {
            // dummy block is excluded.
            return m_block_rank_pairs_.size() / 2 - 1;
        }

        inline uint64_t block_rank(uint64_t block) const {
            // get `next_rank` from `m_block_rank_pairs_`.
            return m_block_rank_pairs_[block * 2];
        }

        // call after BitVector is constructed.
        void build_indices(bool with_select_hints, bool with_select0_hints) {
            // 1. build m_block_rank_pairs_
            std::vector<uint64_t> block_rank_pairs;

            uint64_t next_rank = 0;
            uint64_t cur_sub_rank = 0;
            uint64_t sub_ranks = 0;
            // Initially, next_rank = 0
            block_rank_pairs.push_back(0);
            for (uint64_t i = 0; i < m_bits_.size(); i++) {
                uint64_t word_pop = util::popcount(m_bits_[i]);
                uint64_t shift = i % block_size;
                // TODO: Is there no need to use if-statement?
                if (shift) {
                    // 8 words in a block. We have 8 * 64 popcount at most.
                    // 2^3 * 2^6, so 3 + 6 = 9.
                    sub_ranks <<= 9;
                    sub_ranks |= cur_sub_rank;
                    // sub_ranks contains 7 cur_sub_rank
                }
                next_rank += word_pop;
                cur_sub_rank += word_pop;
                if (shift == block_size - 1) {
                    block_rank_pairs.push_back(sub_ranks);
                    block_rank_pairs.push_back(next_rank);
                    sub_ranks = 0;
                    cur_sub_rank = 0;
                }
            }
            // add last sub_ranks of a block (Maybe dummy).
            uint64_t left = block_size - m_bits_.size() % block_size;
            for (uint64_t i = 0; i < left; ++i) {
                sub_ranks <<= 9;
                sub_ranks |= cur_sub_rank;
            }
            block_rank_pairs.push_back(sub_ranks);
            // a dummy tail block
            if (m_bits_.size() % block_size) {
                block_rank_pairs.push_back(next_rank);
                block_rank_pairs.push_back(0);
            }

            m_block_rank_pairs_.steal(block_rank_pairs);

            // 2. build m_select_hints_
            if (with_select_hints) {
                std::vector<uint64_t> select_hints;

            }

            // 3. build m_select0_hints_
            if (with_select0_hints) {
                std::vector<uint64_t> select0_hints;

            }
        }

        static const uint64_t block_size = 8; // in 64bit words
        static const uint64_t select_ones_per_hint = 64 * block_size * 2; // must be > block_size * 64
        static const uint64_t select_zeros_per_hint = select_ones_per_hint;

        typedef mappable_vector<uint64_t> uint64_vec;
        uint64_vec m_block_rank_pairs_;
        uint64_vec m_select_hints_;
        uint64_vec m_select0_hints_;
    };
}

#endif //PATH_DECOMPOSITION_TRIE_RANK_SELECT_BIT_VECTOR_H
