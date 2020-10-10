//
// Created by Dim Dew on 2020-08-11.
//

#ifndef PATH_DECOMPOSITION_TRIE_PATH_DECOMPOSED_TRIE_H
#define PATH_DECOMPOSITION_TRIE_PATH_DECOMPOSED_TRIE_H

#include "compacted_trie_builder.h"
#include "default_tree_builder.h"
#include "balanced_parentheses_vector.h"

namespace succinct {
    namespace trie {
        // false - CENTROID, true - LEX
        template<bool Lexicographic = false>
        struct DefaultPathDecomposedTrie {
            std::vector<uint16_t> m_labels;      // `L` in paper
            std::vector<uint8_t> m_branches;     // `B` in paper
            BpVector m_bp;                       // `BP` in paper
            // TODO: Use elias-fano encoding later.
            std::vector<size_t> word_positions;

            DefaultPathDecomposedTrie(compacted_trie_builder
                                      <DefaultTreeBuilder<Lexicographic>> &trieBuilder) {
                assert(trieBuilder.is_finish());

                typename DefaultTreeBuilder<Lexicographic>::representation_type
                        root = trieBuilder.get_root();
                root->m_labels.swap(m_labels);
                root->m_branches.swap(m_branches);
                // [double free error] m_bp = BpVector(&root->m_bp, false, true);(fxxk c++!!!!)
                auto tmp = BpVector(&root->m_bp, false, true);
                m_bp.swap(tmp);

                assert(m_labels.back() == DefaultTreeBuilder<Lexicographic>::DELIMITER_FLAG);

                // This method to calculate delimiter position in m_labels is naive.
                // TODO: optimal needed
                word_positions.push_back(0);
                for (int i = 0; i < m_labels.size() - 1; i++) {
                    if (m_labels[i] == DefaultTreeBuilder<Lexicographic>::DELIMITER_FLAG) {
                        word_positions.push_back(i + 1);
                    }
                }
            }

            const std::vector<uint16_t> &get_labels() const {
                return m_labels;
            }

            const std::vector<uint8_t> &get_branches() const {
                return m_branches;
            }

            // TODO: return a const reference OK?
            const BpVector& get_bp() const {
                return m_bp;
            }

            size_t size() const {
                return m_labels.size() + m_branches.size() + m_bp.size();
            }

            void get_branch_idx_by_node_idx(size_t node_idx, size_t& end, size_t& num) const {
                size_t bp_idx = m_bp.select0(node_idx);
                assert(m_bp.rank(bp_idx) >= 2);
                end = m_bp.rank(bp_idx) - 2;
                if (!node_idx) {
                    num = end + 1;
                    return;
                }
                num = bp_idx - m_bp.predecessor0(bp_idx - 1) - 1;
                return;
            }

            // `branch_idx` for `m_bp`
            size_t get_node_idx_by_branch_idx(size_t branch_idx) const {
                assert(branch_idx != 0 && m_bp[branch_idx]);
                return m_bp.rank0(m_bp.successor0(m_bp.find_close(branch_idx) + 1));
            }

            // In rocksdb, we will not use string any more, maybe InternalKey...
            // get the index of `val` in the string set, if not exists return -1.
            int index(const std::string &val) const {
                size_t len = val.size();
                size_t cur_node_idx = 0;
                size_t matching_idx = 0;
                // matching in the trie.
                while (true) {
                    size_t cur_label_idx = word_positions[cur_node_idx];
                    size_t cur_node_bp_idx = m_bp.select0(cur_node_idx);
                    size_t all_branch_num, branch_end;
                    get_branch_idx_by_node_idx(cur_node_idx, branch_end, all_branch_num);
                    size_t cur_branch_idx = (branch_end + 1) - all_branch_num;
                    // matching in a node.
                    while (true) {
                        if (m_labels[cur_label_idx] ==
                            DefaultTreeBuilder<Lexicographic>::DELIMITER_FLAG) {
                            return (matching_idx == len ? cur_node_idx : -1);
                        }
                        if (matching_idx >= len) {
                            return -1;
                        }
                        if (m_labels[cur_label_idx] >> 8 == 1) {
                            assert(m_labels[cur_label_idx + 1] >> 8 == 0);
                            auto branch0 = static_cast<uint8_t>(m_labels[cur_label_idx + 1]);
                            size_t cur_branch_num = static_cast<uint8_t>(m_labels[cur_label_idx]) + 1;

                            if (branch0 == static_cast<uint8_t>(val[matching_idx])) {
                                // update `cur_branch_idx`.
                                cur_branch_idx += cur_branch_num;
                                matching_idx++;
                                cur_label_idx += 2;
                                continue;
                            } else {
                                // check branches.
                                assert(cur_branch_num <= all_branch_num);
                                bool find_branch = false;
                                size_t cur_branch_end = cur_branch_idx + cur_branch_num - 1;
                                while (cur_branch_idx <= cur_branch_end) {
                                    if (m_branches[cur_branch_idx] == static_cast<uint8_t>(val[matching_idx])) {
                                        matching_idx++;
                                        // update `cur_node_idx`.
                                        cur_node_idx = get_node_idx_by_branch_idx(
                                                cur_node_bp_idx + cur_branch_idx - (branch_end + 1)
                                                );
                                        find_branch = true;
                                        break;
                                    }
                                    cur_branch_idx++;
                                }
                                if (find_branch) break;
                                return -1;
                            }
                        } else {
                            if (m_labels[cur_label_idx] == static_cast<uint8_t>(val[matching_idx])) {
                                matching_idx++;
                            } else {
                                return -1;
                            }
                        }
                        cur_label_idx++;
                    }
                }
            }

            // It seems that we can't avoid copy for returning result.
            // get `idx`-th string in string set.
            std::string operator[](size_t idx) const {
                // TODO
                return "";
            }
        };
    }
}

#endif //PATH_DECOMPOSITION_TRIE_PATH_DECOMPOSED_TRIE_H
