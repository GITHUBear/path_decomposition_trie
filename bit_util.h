//
// Created by Dim Dew on 2020-07-18.
//

#ifndef PATH_DECOMPOSITION_TRIE_BIT_UTIL_H
#define PATH_DECOMPOSITION_TRIE_BIT_UTIL_H

#include <cstdint>

namespace succinct {
    namespace util {
        const uint64_t MAGIC_MASK_1 = 0x5555555555555555ULL;
        const uint64_t MAGIC_MASK_2 = 0x3333333333333333ULL;
        const uint64_t MAGIC_MASK_3 = 0x0F0F0F0F0F0F0F0FULL;
        const uint64_t MAGIC_MASK_4 = 0x00FF00FF00FF00FFULL;
        const uint64_t MAGIC_MASK_5 = 0x0000FFFF0000FFFFULL;
//        const uint64_t MAGIC_MASK_6 = 0x00000000FFFFFFFFULL;

        inline uint64_t reverse_bits(uint64_t n) {
            // reverse adjacent bits
            n = ((n >> 1) & MAGIC_MASK_1) | ((n & MAGIC_MASK_1) << 1);
            n = ((n >> 2) & MAGIC_MASK_2) | ((n & MAGIC_MASK_2) << 2);
            n = ((n >> 4) & MAGIC_MASK_3) | ((n & MAGIC_MASK_3) << 4);
            n = ((n >> 8) & MAGIC_MASK_4) | ((n & MAGIC_MASK_4) << 8);
            n = ((n >> 16) & MAGIC_MASK_5) | ((n & MAGIC_MASK_5) << 16);
            n = (n >> 32) | (n << 32);
            return n;
        }
    }
}

#endif //PATH_DECOMPOSITION_TRIE_BIT_UTIL_H
