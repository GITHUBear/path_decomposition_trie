cmake_minimum_required(VERSION 3.14)
project(path_decomposition_trie)

set(CMAKE_CXX_STANDARD 14)

include_directories("/usr/local/include")

link_directories("/usr/local/lib")

add_library(path_decomposition_trie
        library.cpp balanced_parentheses_vector.cpp
        test_bp_vector.cpp)
target_link_libraries(${PROJECT_NAME} gtest)

add_executable(test_bit_vector test_bit_vector.cpp)
target_link_libraries(test_bit_vector gtest)

add_executable(test_bp_vector balanced_parentheses_vector.cpp test_bp_vector.cpp)
target_link_libraries(test_bp_vector gtest)

add_executable(test_path_decomposed_trie test_path_decomposed_trie.cpp)
target_link_libraries(test_path_decomposed_trie gtest)

add_executable(test_varint_encode test_varint_encode.cpp)
target_link_libraries(test_varint_encode gtest)

add_executable(test_pdt_search balanced_parentheses_vector.cpp test_pdt_search.cpp)
target_link_libraries(test_pdt_search gtest)
