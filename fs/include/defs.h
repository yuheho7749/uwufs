#ifndef defs_H
#define defs_H

#include <cstddef>


struct ext4 {
    // size of each block in the filesystem (4 KB)
    static constexpr std::size_t BLOCK_SIZE = 4096;

    // number of direct block pointers in the inode
    static constexpr std::size_t NDIR_BLOCKS = 12;

    // index for the indirect block pointer
    static constexpr std::size_t IND_BLOCK = NDIR_BLOCKS;

    // index for the double-indirect block pointer
    static constexpr std::size_t DIND_BLOCK = IND_BLOCK + 1;

    // index for the triple-indirect block pointer
    static constexpr std::size_t TIND_BLOCK  = DIND_BLOCK  + 1;

    // total number of block pointers in the inode
    static constexpr std::size_t N_BLOCKS = TIND_BLOCK + 1;

    // size of each inode in the filesystem (256 bytes)
    static constexpr std::size_t INODE_SIZE = 256;
};

#endif