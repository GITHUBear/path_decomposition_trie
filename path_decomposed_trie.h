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

            struct LabelEnumerator {


                friend struct DefaultPathDecomposedTrie;

            };

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



            // In rocksdb, we will not use string any more, maybe InternalKey...
            // get the index of `val` in the string set, if not exists return -1.
            size_t index(const std::string &val) const {
                // TODO
                size_t len = val.size();
                size_t cur_node_idx = 0;

                while (true) {

                }
                return 0;
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
