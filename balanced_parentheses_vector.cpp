//
// Created by Dim Dew on 2020-07-23.
//

#include "balanced_parentheses_vector.h"

namespace succinct {
    //
    // `pos`: bit index
    uint64_t BpVector::find_open(uint64_t pos) const {

    }

    void BpVector::build_min_tree() {
        if (!size()) return;

        std::vector<block_min_excess_t> block_excess_min;
        excess_t cur_block_min = 0, cur_superblock_excess = 0;
        for (uint64_t sub_block = 0; sub_block < m_bits_.size(); ++sub_block) {
            if (sub_block % bp_block_size == 0) {
                if (sub_block % (bp_block_size * superblock_size) == 0) {
                    cur_superblock_excess = 0;
                }
                if (sub_block) {
                    assert(cur_block_min >= std::numeric_limits<block_min_excess_t>::min());
                    assert(cur_block_min <= std::numeric_limits<block_min_excess_t>::max());
                    block_excess_min.push_back((block_min_excess_t)cur_block_min);
                    cur_block_min = cur_superblock_excess;
                }
            }
            uint64_t word = m_bits_[sub_block];
            uint64_t mask = 1ULL;
            // for last block stop at bit boundary
            uint64_t n_bits =
                    (sub_block == m_bits_.size() - 1 && size() % 64)
                    ? size() % 64
                    : 64;

            for (uint64_t i = 0; i < n_bits; ++i) {
                cur_superblock_excess += (word & mask) ? 1 : -1;
                cur_block_min = std::min(cur_block_min, cur_superblock_excess);
                mask <<= 1;
            }
        }
        // Flush last block mins
        assert(cur_block_min >= std::numeric_limits<block_min_excess_t>::min());
        assert(cur_block_min <= std::numeric_limits<block_min_excess_t>::max());
        block_excess_min.push_back((block_min_excess_t)cur_block_min);

        size_t n_blocks = (m_bits_.size() + bp_block_size - 1) / bp_block_size;
        assert(n_blocks == block_excess_min.size());

        size_t n_superblocks = (n_blocks + superblock_size - 1) / superblock_size;

        size_t n_complete_leaves = 1;
        // TODO: use util::msb to calc `n_complete_leaves`.
        while (n_complete_leaves < n_superblocks) n_complete_leaves <<= 1;
        m_internal_nodes_ = n_complete_leaves;
        size_t treesize = m_internal_nodes_ + n_superblocks;

        std::vector<excess_t> superblock_excess_min(treesize);

        // Fill in the leaves of the tree
        for (size_t superblock = 0; superblock < n_superblocks; ++superblock) {
            excess_t cur_super_min = static_cast<excess_t>(size());
            excess_t superblock_excess = get_block_excess(superblock * superblock_size);

            for (size_t block = superblock * superblock_size;
                 block < std::min((superblock + 1) * superblock_size, n_blocks);
                 ++block) {
                cur_super_min = std::min(cur_super_min, superblock_excess + block_excess_min[block]);
            }
            assert(cur_super_min >= 0 && cur_super_min < excess_t(size()));

            superblock_excess_min[m_internal_nodes_ + superblock] = cur_super_min;
        }

        // fill in the internal nodes with past-the-boundary values
        // (they will also serve as sentinels in debug)
        for (size_t node = 0; node < m_internal_nodes_; ++node) {
            superblock_excess_min[node] = static_cast<excess_t>(size());
        }

        // Fill bottom-up the other layers: each node updates the parent
        for (size_t node = treesize - 1; node > 1; --node) {
            size_t parent = node / 2;
            superblock_excess_min[parent] = std::min(superblock_excess_min[parent],
                                                     superblock_excess_min[node]);
        }

        m_block_excess_min_.steal(block_excess_min);
        m_superblock_excess_min_.steal(superblock_excess_min);
    }
}