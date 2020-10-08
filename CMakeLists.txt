cmake_minimum_required(VERSION 3.14)
project(path_decomposition_trie)

set(CMAKE_CXX_STANDARD 14)

include_directories("/usr/local/include")

link_directories("/usr/local/lib")

add_library(path_decomposition_trie
        library.cpp library.h
        bit_vector.h bit_util.h mappable_vector.h
        test_bit_vector.cpp rank_select_bit_vector.h
        balanced_parentheses_vector.h balanced_parentheses_vector.cpp
        test_bp_vector.cpp compacted_trie_builder.h
        bit_strings.h default_tree_builder.h test_path_decomposed_trie.cpp path_decomposed_trie.h varint_encode.h test_varint_encode.cpp)
target_link_libraries(${PROJECT_NAME} gtest)

add_executable(test_bit_vector
        bit_vector.h bit_util.h mappable_vector.h test_bit_vector.cpp)
target_link_libraries(test_bit_vector gtest)

add_executable(test_bp_vector
        balanced_parentheses_vector.h balanced_parentheses_vector.cpp test_bp_vector.cpp)
target_link_libraries(test_bp_vector gtest)

add_executable(test_path_decomposed_trie
        compacted_trie_builder.h default_tree_builder.h test_path_decomposed_trie.cpp)
target_link_libraries(test_path_decomposed_trie gtest)

add_executable(test_varint_encode
        varint_encode.h test_varint_encode.cpp)
target_link_libraries(test_varint_encode gtest)
