//
// Created by Dim Dew on 2020-07-30.
//

#ifndef PATH_DECOMPOSITION_TRIE_COMPACTED_TRIE_BUILDER_H
#define PATH_DECOMPOSITION_TRIE_COMPACTED_TRIE_BUILDER_H

#include <vector>
#include <string>
#include <algorithm>

namespace succinct {
    namespace trie {
        template <typename TreeBuilder>
        struct compacted_trie_builder {
        public:
            compacted_trie_builder(): is_finish_(false) {
                assert(node_stack.empty() && last_string.empty());
            }

            void append(std::vector<uint8_t>& bytes) {
                assert(!is_finish_);
                if (bytes.empty()) return;

                if (node_stack.empty()) {
                    // first bytes
                    last_string.assign(bytes.begin(), bytes.end());
                    node_stack.push_back(node(0, last_string.size()));
                } else {
                    size_t min_len = std::min(bytes.size(), last_string.size());
                    auto match_res =
                            std::mismatch(
                                    bytes.begin(),
                                    bytes.begin() + min_len,
                                    last_string.begin());

                    // Prefix-free and non-duplicate
                    assert(match_res.first != bytes.end() &&
                           match_res.second != last_string.end());
                    // Sorted
                    assert(*match_res.first > *match_res.second);

                    size_t mismatch_idx = match_res.first - bytes.begin();
                    size_t split_node_idx = 0;
                    // find the node to split
                    while (mismatch_idx > node_stack[split_node_idx].get_end()) {
                        assert(split_node_idx < node_stack.size());
                        ++split_node_idx;
                    }
                    node& split_node = node_stack[split_node_idx];
                    assert(mismatch_idx >= split_node.path_len &&
                           mismatch_idx <= split_node.get_end());
                    // pop the node after split_node in node_stack
                    for (size_t idx = node_stack.size() - 1; idx > split_node_idx; idx--) {
                        node& child = node_stack[idx];
                        typename TreeBuilder::representation_type subtrie =
                                builder.node(child.children, &last_string[0], child.path_len, child.skip);
                        uint8_t branch_byte = last_string[child.path_len - 1];
                        node_stack[idx - 1].children.push_back(std::make_pair(branch_byte, subtrie));
                    }

                }
            }

            void finish() {
                assert(!is_finish_);
                assert(!node_stack.empty());
            }

            void build(TreeBuilder& builder, const std::vector<std::string>& strings) {
                if (strings.empty()) return;


            }
        private:
            struct node {
                node()
                        : skip(-1)
                {}

                node(size_t path_len_, size_t skip_)
                        : path_len(path_len_)
                        , skip(skip_)
                {}

                size_t get_end() { return path_len + skip; }

                size_t path_len;
                size_t skip;

                typedef std::pair<uint8_t, typename TreeBuilder::representation_type> subtrie;
                std::vector<subtrie> children;
            };

            bool is_finish_;            // Is building process finish?

            TreeBuilder& builder;
            std::vector<node> node_stack;
            std::vector<uint8_t> last_string;

        };
    }
}

#endif //PATH_DECOMPOSITION_TRIE_COMPACTED_TRIE_BUILDER_H
