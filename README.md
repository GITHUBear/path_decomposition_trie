# Path Decomposed Trie

## 目的

本库受到了 [*ot/path_decomposed_tries*](https://github.com/ot/path_decomposed_tries) 
的启发，并基于该库进行了一定的修改以达到如下目的：

- 更准确且易于维护的代码逻辑；
- 少量地对代码效率进行了提升；
- 便于在 `rocksdb` 中使用本库；

Path Decomposed Trie 将字符串集合 S 构成的字典树通过 succint data structure
技术实现了在较小的空间使用的情况下保存字符串集合 S 的信息，
所以在 `rocksdb` 使用可以带来如下优势：

- 取代 `Filter Block` 以做到不存在 positive negative 的过滤，
同时占用的数据量在可接受范围；
- 在 level 较高的 sstable 当中使用将更有可能减小数据文件的大小，
从而在一定程度减小写放大；
- 取代 `Filter Block` 后带来的低误判率，可以有效减小读放大；

(对于是否需要取代 `Index Block`，我觉得可能不太需要，因为建立 Path Decomposed Trie 
复杂度相比于 `rocksdb` 中原先的 `Index Block` 来说还是较高的(简答来讲就是预处理复杂度较高)，而且读性能上，如果 Path Decomposed Trie 
采用的是 centroid path decomposed 方式读效率可能不及直接二分查找来的高效)

## 基本算法以及结构

### bit util (in bit_util.h)

基于 [*ot/succint/broadword.hpp*](https://github.com/ot/succinct/blob/master/broadword.hpp) 
进行修改，该头文件使用 uint64_t 作为操作对象，同时对 uint64_t 中的8个 bytes 进行并行的 +、-、compare、unsigned compare
等操作

其中比较核心的算法思路如下：

- 通过位操作来实现8个bytes上的 divide & conquer 算法，如 byte_count(统计每一个byte上的1-bit数量) 
、bit_count(统计64位中的1-bit数量) 以及 reverse_bits/bytes(位倒置)；
- 通过 de bruijn 序列(数学表示为 B(M, N), M表示序列中的每一个元素能够取值M种，N表示子序列长度，简单来说 de bruijn 
序列能够将 M^N 种的 N 长度 M 阶序列编码在长度为 M^N 的序列中)，该序列在每一个索引位置处的 N 长度子串都是唯一的，
利用这个一一对应关系建立一张映射表，可以在 O(1) 的复杂度计算仅有一位置为1的 uint64_t 中该置1位所在的索引(详见bit_position)；
- 利用逻辑分析表处理逐 bytes 的有符号大小比较，如 leq_bytes；

#### 修改

- 将核心算法第3点中的逻辑表达式换成了更易于理解的等价逻辑表达式：

``` C++
// let r = (w_ks | indicator) - (w_ranks & ~indicator)
// let k = w_ks and x = w_ranks
// use `w_flags` to represent if w_ks >= w_ranks in each 9-bits
// We have the logistic statement:
//   k(r|(~r~x)) & ~k(r~x)
// = krx | k~x | ~kr~x
// = r(kx & ~k~x) | k~x
// = r & (~(k ^ x)) | (k & ~x)
const uint64_t r = (w_ks | indicator) - (w_ranks & (~indicator));
uint64_t w_flags =
        ((r & (~(w_ks ^ w_ranks))) | (w_ks & (~w_ranks))) & indicator;
```

- 修改原先实现中关于 bytes 的无符号比较的实现错误：

origin:

```C++
inline uint64_t leq_step_8(uint64_t x, uint64_t y)
{
    return ((((y | msbs_step_8) - (x & ~msbs_step_8)) ^ (x ^ y)) & msbs_step_8) >> 7;
}
```

上面的操作中在 x,y 对应的 byte 为同号的时候结果是正确的，但是在 x,y 异号的时候就存在错误，
不过因为x，y的每个byte有取值范围（y的每一个byte最小也只有-8，x的最大只有64），所以 x,y 
中对应 byte 异号时有符号比较错误的问题实际上在代码运行过程中不会出现，不过我还是进行了修改：

now:

```C++
inline uint64_t leq_bytes(uint64_t x, uint64_t y) {
    const uint64_t indicator = 0x80ULL * BYTE_UNIT;
    return (((((y | indicator) - (x & (~indicator))) & (~(x ^ y))) | (x & (~y))) & indicator) >> 7;
}
```

### bit vector 族

在本库中需要三种不同抽象程度的 bit vector，如下：

- bit vector(in bit_vector.h)：提供最基本的位操作，如截取某个位置开始特定长度的位序列、
确定前驱/后继0或者1的位置等等；
- rank & select bit vector(in rank_select_bit_vector.h)：在 bit vector 的基础上，通过分块算法提供接近 
O(1) 复杂度的 rank(index) 和 select(rank) 操作；
- balanced parentheses vector(in balanced_parentheses_vector.h)：在 rank & select bit vector 的基础上，
采用 block、superblock 的两级分块机制，再利用 RMQ 算法提供 O(1)(不过常数比较大) 的查找与 "(" 匹配的 ")" 所在的位置
(或者与 ")" 匹配的 "(" 位置)

三者关系通过继承表达：

```plain
                +------------+
                | bit_vector |
                +------------+
                      A
                      |
          +-------------------------+
          | rank & select bit vector|
          +-------------------------+
                      A
                      |
         +----------------------------+
         | balanced parentheses vector|
         +----------------------------+
```

`ot/path_decomposed_tries` 的代码中对每一个 class 的结构以及作用都没有给出阐述，
甚至对每一个 class 的成员都没有解释其意义，现将这些内容梳理如下：

#### BitVector

BitVector 结构图如下所示：

```plain

                        +--------------+                          
                        |  +----------+|
                        |  |enumerator||
                        |  +----------+|
                        |              |
    +-------—--+        |  +----------+|
    |   Bit    |  move  |  |   unary  ||
    |  Vector  |------->|  |enumerator||
    |  Builder |        |  +----------+|
    +----------+        |              |
                        |   BitVector  |
                        |              |
                        +--------------+
                        
```

BitVectorBuilder作用有点类似 java 中的 StringBuilder，支持 append，set 
等修改一个位序列的方法，在 BitVectorBuilder 完成 build 任务后调用它的 
`move_bits` 方法通过移动构造一个 BitVector，在 BitVector 中的 enumerator 以及
unary enumerator 扮演了迭代器的角色，enumerator 负责逐 bit 地迭代，而 unary 
enumerator 仅在置为1的位上进行迭代

#### RsBitVector

RsBitVector 的基本结构为：

```C++
// In BitVector, bits are combined in the following format:
//
//         64bit  64bit        64bit  64bit
//       +------+------+     +------+------+
//       |      |      | ... |      |      |
//       +------+------+     +------+------+
//
// While in RsBitVector, 64bits are organized logically in the following format:
//
//                 +-----+-----+-----+
//       block 0   |     |     |     | ...  length: block_size
//                 +-----+-----+-----+
//                 A                     A
//                 |                     |
//                 next_rank             sub_rank
//                 (calculated at        (calculated at
//                 block head)           block tail)
//
//       block 1    ......
//
//         ...
//
//   dummy tail block
//          A
//          |
// at last, a dummy tail block is used to save the number of one-bits in `next_rank`
//
```

其中，next_rank 记录了每一个 Block 之前的 bit 中置为1的数量，而 sub_rank 
是相对于每一个 Block 起始位置而言的，每隔一个 uint64_t 计算一次的 rank 
信息序列

另外，在 `ot/path_decomposed_tries` 的实现当中，还引入了 m_select_hints_ 序列来
加速 select 的求解，其原理比较简单，就是每隔一定的 rank 值，记录下落在边界的 block 索引，
由于这一点有利于提升读性能，本实现中予以了沿用

#### BpVector

BpVector 的基本结构为：

```C++
//
// In a sequence of n balanced parentheses (BP), each open parenthesis
// "(" can be associated to its mate ")". Operations FindClose and FindOpen can be defined,
// which find the position of the mate of respectively an open or close parenthesis.
// The sequence can be represented as a bit vector, where 1 represents "(" and 0 represents ")"
//
// In BpVector, 64bits are organized logically in the following format:
//
//                                              +---- block_min_excess(calculated at the beginning of block)
//                                              |
//                                              V
//           +-----+-----+-----+-----+          +-----+-----+-----+-----+
//           |     |     |     |     |    ...   |     |     |     |     |   ...
//           +-----+-----+-----+-----+          +-----+-----+-----+-----+
//           |<-   bp_block_size   ->|          |<-   bp_block_size   ->|
//           |<-                  superblock_size                     ->|
//
// `m_block_excess_min_` and `m_superblock_excess_min_` maintain minimum excess
// (the number of mismatched "(") in different logical level:
//
//                                 4superblock
//                                 excess
//                                 min
//                          /                        \
//                         /                          \
//                        /                            \
//                       /                              \
//
//                2superblock                         2superblock
//                excess                              excess
//                min                                 min
//             /                \                 /                 \
//            /                  \               /                   \
//         1superblock      1superblock       1superblock       1superblock
//         excess           excess            excess            excess
//         min              min               min               min
//     +--------------+  +--------------+  +--------------+  +--------------+
//     |superblock    |  |superblock    |  |superblock    |  |superblock    |
//     | +---+   +---+|  | +---+   +---+|  | +---+   +---+|  | +---+   +---+|
//     | |blk|...|blk||  | |blk|...|blk||  | |blk|...|blk||  | |blk|...|blk||  ...
//     | +---+   +---+|  | +---+   +---+|  | +---+   +---+|  | +---+   +---+|
//     +--------------+  +--------------+  +--------------+  +--------------+
//
// `m_superblock_excess_min_` is used in RMQ algorithm.
//
```

BpVector 的算法流程非常复杂，不仅是使用了初看起来非常意义不明的成员，而且后续采用的 RMQ 
算法也并不是之前我常使用的实现方式，首先解释一下成员意义：

- excess：指的是一个区间中 "(" 的数量减去 ")" 的数量；
- m_block_excess_min_：以 block 所在的 superblock 作为起始位置，也就是每一个 superblock 
开始位置本值是初始化为0的，记录 superblock 中每一个 block 中最小的 excess 值；
- m_super_excess_min_：实际上是一个用数组维护的完全二叉树，每一个元素表示的是 2^n 
范围内的 superblock 中最小的 excess 值，需要说明的是 `本值是相对于整个 bit 序列的起始位置而言的`；

至于，FindOpen 和 FindClose 操作为什么需要 min excess 和 RMQ 算法，原因如下：

- excess 值实际上对应了括号序列构成的一个二叉树的树高；
- ")" 表示二叉树中一个节点 DFS 遍历结束， "(" 表示二叉树中一个节点开始 DFS 遍历；
- 如果从给定的 ")" 开始向序列前到某个位置之间的区间内的最小 excess 比 ")" 位置处的
excess 小，说明与之对应的 "(" 落在这个范围 (根据二叉树的 DFS 顺序，
很显然说明这个范围间存在一个 ")" 所代表节点的父节点，也就是在这个父节点时还没有开始对子节点的 DFS，
必然有要寻找的 "(" 落在这个范围内)
- 以上都说明这是个区间最值询问问题

### Path Decomposed Trie

这部分就是核心的创建 Path Decomposed Trie 的算法了

