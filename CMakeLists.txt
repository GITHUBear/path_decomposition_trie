cmake_minimum_required(VERSION 3.14)
project(path_decomposition_trie)

set(CMAKE_CXX_STANDARD 14)

include_directories("/usr/local/include")

link_directories("/usr/local/lib")

add_library(path_decomposition_trie
        library.cpp library.h
        bit_vector.h bit_util.h mappable_vector.h test_bit_vector.cpp rank_select_bit_vector.h)
target_link_libraries(${PROJECT_NAME} gtest)

add_executable(test_bit_vector
        bit_vector.h bit_util.h mappable_vector.h test_bit_vector.cpp)
target_link_libraries(test_bit_vector gtest)
