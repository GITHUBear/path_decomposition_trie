cmake_minimum_required(VERSION 3.14)
project(path_decomposition_trie)

set(CMAKE_CXX_STANDARD 14)

add_library(path_decomposition_trie library.cpp library.h bit_vector.h bit_util.h)