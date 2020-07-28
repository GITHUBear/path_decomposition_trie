//
// Created by Dim Dew on 2020-07-23.
//

#include "balanced_parentheses_vector.h"

namespace succinct {
    // excess tables for excess infos in a byte.
    class excess_tables
    {
    public:
        excess_tables() {
            for (int c = 0; c < 256; ++c) {
                for (uint8_t i = 0; i < 9; ++i) {
                    m_fwd_pos[c][i] = 0;
                    m_bwd_pos[c][i] = 0;
                }
                // populate m_fwd_pos, m_fwd_min, and m_fwd_min_idx
                int excess = 0;
                m_fwd_min[c] = 0;
                m_fwd_min_idx[c] = 0;

                for (char i = 0; i < 8; ++i) {
                    if ((c >> i) & 1) { // opening
                        ++excess;
                    } else { // closing
                        --excess;
                        if (excess < 0 &&
                            m_fwd_pos[c][-excess] == 0) { // not already found
                            m_fwd_pos[c][-excess] = uint8_t(i + 1);
                        }
                    }

                    if (-excess > m_fwd_min[c]) {
                        m_fwd_min[c] = uint8_t(-excess);
                        m_fwd_min_idx[c] = uint8_t(i + 1);
                    }
                }
                m_fwd_exc[c] = (char)excess;

                // populate m_bwd_pos and m_bwd_min
                excess = 0;
                m_bwd_min[c] = 0;

                for (uint8_t i = 0; i < 8; ++i) {
                    if ((c << i) & 128) { // opening
                        ++excess;
                        if (excess > 0 &&
                            m_bwd_pos[c][(uint8_t)excess] == 0) { // not already found
                            m_bwd_pos[c][(uint8_t)excess] = uint8_t(i + 1);
                        }
                    } else { // closing
                        --excess;
                    }

                    m_bwd_min[c] = uint8_t(std::max(excess, (int)m_bwd_min[c]));
                }
            }
        }

        char m_fwd_exc[256];

        uint8_t m_fwd_pos[256][9];
        uint8_t m_bwd_pos[256][9];

        uint8_t m_bwd_min[256];    // maximum mismatched "(" in word
        uint8_t m_fwd_min[256];    // maximum mismatched ")" in word

        uint8_t m_fwd_min_idx[256];
    };

    const static excess_tables tables;

    bool find_open_in_word(
            uint64_t word, uint64_t byte_counts,
            BpVector::excess_t cur_exc, uint64_t& ret) {
        assert(cur_exc > 0 && cur_exc <= 64);
        const uint64_t rev_byte_counts = util::reverse_bytes(byte_counts);
        const uint64_t cum_exc_step_8 =
                (uint64_t(cur_exc) - ((2 * rev_byte_counts - 8 * util::BYTE_UNIT) << 8)) * util::BYTE_UNIT;

        // maximum mismatched "(" in word
        uint64_t max_exc_step_8 = 0;
        for (size_t i = 0; i < 8; ++i) {
            size_t shift = i * 8;
            max_exc_step_8 |= ((uint64_t)(tables.m_bwd_min[(word >> (64 - shift - 8)) & 0xFF])) << shift;
        }

        const uint64_t has_result = util::leq_bytes(cum_exc_step_8, max_exc_step_8);

        unsigned long shift;
        if (util::lsb(has_result, shift)) {
            uint8_t bit_pos = tables.m_bwd_pos[(word >> (64 - shift - 8)) & 0xFF][(cum_exc_step_8 >> shift) & 0xFF];
            assert(bit_pos > 0);
            ret = 64 - (shift + bit_pos);
            return true;
        }
        return false;
    }

    // search in the block index range `block_offset + [0, start - 1]`
    inline bool BpVector::find_open_in_block(
            uint64_t block_offset, BpVector::excess_t excess, uint64_t start, uint64_t& ret) const {
        // `excess` is the number of ")" that is mismatched.
        // `excess` > start * 64 means that even if all the remaining bits are 1,
        // the "(" we need to look for is not in local block.
        // It also excludes the case of start = 0.
        if (excess > excess_t(start * 64)) {
            return false;
        }
        // In the case where `find_open_in_word` does not find the result:
        assert(excess >= 0);

        for (uint64_t sub_block_offset = start - 1; sub_block_offset + 1 > 0; --sub_block_offset) {
            assert(excess > 0);
            uint64_t sub_block = block_offset + sub_block_offset;
            uint64_t word = m_bits_[sub_block];
            uint64_t byte_counts = util::byte_counts(word);
            if (excess <= 64) {
                if (find_open_in_word(word, byte_counts, excess, ret)) {
                    ret += sub_block * 64;
                    return true;
                }
            }
            excess -= static_cast<excess_t>(2 * util::bytes_sum(byte_counts) - 64);
        }
        return false;
    }

    template <int direction>
    inline bool BpVector::search_block_in_superblock(
            uint64_t block, excess_t excess, size_t& found_block) const {
        size_t superblock = block / superblock_size;
        excess_t superblock_excess = get_block_excess(superblock * superblock_size);
        if (direction) {
            for (size_t cur_block = block;
                 cur_block < std::min((superblock + 1) * superblock_size, (size_t)m_block_excess_min_.size());
                 ++cur_block) {
                if (excess >= superblock_excess + m_block_excess_min_[cur_block]) {
                    found_block = cur_block;
                    return true;
                }
            }
        } else {
            for (size_t cur_block = block;
                 cur_block + 1 >= (superblock * superblock_size) + 1;
                 --cur_block) {
                if (excess >= superblock_excess + m_block_excess_min_[cur_block]) {
                    found_block = cur_block;
                    return true;
                }
            }
        }

        return false;
    }

    template <int direction>
    inline uint64_t BpVector::search_min_tree(uint64_t block, excess_t excess) const {
//        size_t found_block = -1U;
//        if (search_block_in_superblock<direction>(block, excess, found_block)) {
//            return found_block;
//        }
//
//        size_t cur_superblock = block / superblock_size;
//        size_t cur_node = m_internal_nodes + cur_superblock;
//        while (true) {
//            assert(cur_node);
//            bool going_back = (cur_node & 1) == direction;
//            if (!going_back) {
//                size_t next_node = direction ? (cur_node + 1) : (cur_node - 1);
//                if (in_node_range(next_node, excess)) {
//                    cur_node = next_node;
//                    break;
//                }
//            }
//            cur_node /= 2;
//        }
//
//        assert(cur_node);
//
//        while (cur_node < m_internal_nodes) {
//            uint64_t next_node = cur_node * 2 + (1 - direction);
//            if (in_node_range(next_node, excess)) {
//                cur_node = next_node;
//                continue;
//            }
//
//            next_node = direction ? (next_node + 1) : (next_node - 1);
//            // if it is not one child, it must be the other
//            assert(in_node_range(next_node, excess));
//            cur_node = next_node;
//        }
//
//        size_t next_superblock = cur_node - m_internal_nodes_;
//        bool ret = search_block_in_superblock<direction>(next_superblock * superblock_size + (1 - direction) * (superblock_size - 1),
//                                                         excess, found_block);
//        assert(ret); (void)ret;
//
//        return found_block;
    }

    //
    // `pos`: bit index
    uint64_t BpVector::find_open(uint64_t pos) const {
        assert(pos);
        uint64_t ret = -1U;
        // Search in current word
        uint64_t word_pos = (pos / 64);
        uint64_t len = pos % 64;
        // Rest is padded with "close"
        uint64_t shifted_word = -(len != 0) & (m_bits_[word_pos] << (64 - len));
        uint64_t byte_counts = util::byte_counts(shifted_word);

        excess_t word_exc = 1;
        if (find_open_in_word(shifted_word, byte_counts, word_exc, ret)) {
            ret += pos - 64;
            return ret;
        }

        // Otherwise search in the local block
        uint64_t block = word_pos / bp_block_size;
        uint64_t block_offset = block * bp_block_size;
        uint64_t sub_block = word_pos % bp_block_size;
        uint64_t local_rank = util::bytes_sum(byte_counts); // no need to subtract the padding
        excess_t local_excess = -static_cast<excess_t>((2 * local_rank) - len);
        if (find_open_in_block(block_offset, local_excess + 1, sub_block, ret)) {
            return ret;
        }

        // Otherwise, find the first appropriate block
        excess_t pos_excess = excess(pos) - 1;
        uint64_t found_block = search_min_tree<0>(block - 1, pos_excess);
        uint64_t found_block_offset = found_block * bp_block_size;
        // Since search is backwards, have to add the current block
        excess_t found_block_excess = get_block_excess(found_block + 1);

        // Search in the found block
        bool found = find_open_in_block(found_block_offset, found_block_excess - pos_excess, bp_block_size, ret);
        assert(found); (void)found;
        return ret;
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