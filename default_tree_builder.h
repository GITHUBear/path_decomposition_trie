//
// Created by Dim Dew on 2020-08-07.
//

#ifndef PATH_DECOMPOSITION_TRIE_DEFAULT_TREE_BUILDER_H
#define PATH_DECOMPOSITION_TRIE_DEFAULT_TREE_BUILDER_H

#include <vector>
#include <memory>

class DefaultTreeBuilder {
public:
    // node label is encoded by uint16_t:
    //
    //        special_char_flag (uint8_t: 0/1) + char/branching number (uint8_t)
    //
    static const size_t SPECIAL_CHAR_FLAG = 256;

    struct subtree {
        // infos for decomposition trie
        std::vector<uint16_t> m_decompostion_path_label;
        std::vector<uint8_t> m_decompostion_branches;
        // infos for origin compacted trie

        std::vector<uint8_t> m_branches;
    };

    typedef std::shared_ptr<subtree> representation_type;
private:
    representation_type root_node;
};

#endif //PATH_DECOMPOSITION_TRIE_DEFAULT_TREE_BUILDER_H
