//
// Created by Dim Dew on 2020-07-18.
//

#ifndef PATH_DECOMPOSITION_TRIE_BIT_VECTOR_H
#define PATH_DECOMPOSITION_TRIE_BIT_VECTOR_H

#include <vector>

#include "bit_util.h"

namespace succinct {
    class BitVectorBuilder {
    public:
        typedef std::vector<uint64_t> bits_t;
        const uint64_t WORD_SIZE = 64;

        // noncopyable because we have a pointer member
        BitVectorBuilder &operator=(const BitVectorBuilder &builder) = delete;

        BitVectorBuilder(const BitVectorBuilder &builder) = delete;

        // new a BitVectorBuilder containing a vector of `size` bits
        // the elements of the vector are all 0
        // if `set` is set, the elements of the vector are all 0 in bits
        BitVectorBuilder(uint64_t size, bool set = 0)
                : m_size_(size) {
            m_bits_.resize(word_for(size), uint64_t(-set));
            if (size) {
                m_cur_word_ = &m_bits_.back();
                if (set && size % WORD_SIZE) {
                    *m_cur_word_ >>= WORD_SIZE - (size % WORD_SIZE);
                }
            }
        }

        void reserve(uint64_t size) {
            m_bits_.reserve(word_for(size));
        }

        // append a `b` bit into vector
        inline void push_back(bool b) {
            uint64_t pos_in_word = m_size_ % WORD_SIZE;
            if (pos_in_word == 0) {
                m_bits_.push_back(0);
                m_cur_word_ = &m_bits_.back();
            }
            *m_cur_word_ |= (uint64_t) b << pos_in_word;
            ++m_size_;
        }

        // set `b` bit at `pos` index
        inline void set(uint64_t pos, bool b) {
            uint64_t word = pos / WORD_SIZE;
            uint64_t pos_in_word = pos % WORD_SIZE;

            m_bits_[word] &= ~(uint64_t(1) << pos_in_word);
            m_bits_[word] |= uint64_t(b) << pos_in_word;
        }

        // set `bits` of length `len` at `pos` index
        // len >= 0 && len <= 64
        inline void set_bits(uint64_t pos, uint64_t bits, size_t len) {
            assert(pos + len <= size());
            // check there are no spurious bits
            assert(len == WORD_SIZE || (bits >> len) == 0);
            if (!len) return;
            uint64_t mask = (len == WORD_SIZE) ? uint64_t(-1) : ((uint64_t(1) << len) - 1);
            uint64_t word = pos / WORD_SIZE;
            uint64_t pos_in_word = pos % WORD_SIZE;

            m_bits_[word] &= ~(mask << pos_in_word);
            m_bits_[word] |= bits << pos_in_word;

            uint64_t stored = WORD_SIZE - pos_in_word;
            if (stored < len) {
                m_bits_[word + 1] &= ~(mask >> stored);
                m_bits_[word + 1] |= bits >> stored;
            }
        }

        // append `bits` of length `len`
        // len >= 0 && len <= 64
        inline void append_bits(uint64_t bits, size_t len) {
            // check there are no spurious bits
            assert(len == WORD_SIZE || (bits >> len) == 0);
            if (!len) return;
            uint64_t pos_in_word = m_size_ % WORD_SIZE;
            m_size_ += len;
            if (pos_in_word == 0) {
                m_bits_.push_back(bits);
            } else {
                *m_cur_word_ |= bits << pos_in_word;
                if (len > WORD_SIZE - pos_in_word) {
                    m_bits_.push_back(bits >> (WORD_SIZE - pos_in_word));
                }
            }
            m_cur_word_ = &m_bits_.back();
        }

        // append n `0` bits
        inline void zero_extend(uint64_t n) {
            m_size_ += n;
            uint64_t needed = word_for(m_size_) - m_bits_.size();
            if (needed) {
                m_bits_.insert(m_bits_.end(), needed, 0);
                m_cur_word_ = &m_bits_.back();
            }
        }

        // append n `1` bits
        inline void one_extend(uint64_t n) {
            while (n >= 64) {
                append_bits(uint64_t(-1), WORD_SIZE);
                n -= WORD_SIZE;
            }
            if (n) {
                append_bits(uint64_t(-1) >> (WORD_SIZE - n), n);
            }
        }

        // append another BitVectorBuilder's m_bits
        void append(const BitVectorBuilder &rhs) {
            if (!rhs.size()) return;

            uint64_t pos = m_bits_.size();
            uint64_t shift = size() % WORD_SIZE;
            m_size_ = size() + rhs.size();
            m_bits_.resize(word_for(m_size_));

            if (shift == 0) { // word-aligned, easy case
                std::copy(rhs.m_bits_.begin(), rhs.m_bits_.end(),
                          m_bits_.begin() + ptrdiff_t(pos));
            } else {
                uint64_t *cur_word = &m_bits_.front() + pos - 1;
                for (size_t i = 0; i < rhs.m_bits_.size() - 1; ++i) {
                    uint64_t w = rhs.m_bits_[i];
                    *cur_word |= w << shift;
                    *++cur_word = w >> (WORD_SIZE - shift);
                }
                *cur_word |= rhs.m_bits_.back() << shift;
                if (cur_word < &m_bits_.back()) {
                    *++cur_word = rhs.m_bits_.back() >> (WORD_SIZE - shift);
                }
            }
            m_cur_word_ = &m_bits_.back();
        }

        // reverse in place
        void reverse() {
            uint64_t shift = WORD_SIZE - (size() % WORD_SIZE);

            uint64_t remainder = 0;
            for (size_t i = 0; i < m_bits_.size(); ++i) {
                uint64_t cur_word;
                if (shift != WORD_SIZE) { // this should be hoisted out
                    cur_word = remainder | (m_bits_[i] << shift);
                    remainder = m_bits_[i] >> (WORD_SIZE - shift);
                } else {
                    cur_word = m_bits_[i];
                }
                // reverse bits in a word
                m_bits_[i] = succinct::util::reverse_bits(cur_word);
            }
            assert(remainder == 0);
            std::reverse(m_bits_.begin(), m_bits_.end());
        }

        void swap(BitVectorBuilder &other) {
            m_bits_.swap(other.m_bits_);
            std::swap(m_size_, other.m_size_);
            std::swap(m_cur_word_, other.m_cur_word_);
        }


        uint64_t size() const {
            return m_size_;
        }

    private:
        inline uint64_t word_for(uint64_t size) {
            return (size + WORD_SIZE - 1) / WORD_SIZE;
        }

        bits_t m_bits_;
        uint64_t m_size_;
        uint64_t *m_cur_word_;
    };

    class BitVector {
    public:

    private:
        uint64_t m_size_;
    };
}

#endif //PATH_DECOMPOSITION_TRIE_BIT_VECTOR_H
