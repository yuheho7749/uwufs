#ifndef INode_h
#define INode_h

#include "../uwufs.h"


class DataBlockIterator;    // forward declaration


// This class is a wrapper around the uwufs_inode struct
// It never writes to disk, only modifies the in-memory inode
class INode {
public:
    struct IndirectBlock {
        uwufs_blk_t block_nos[UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t)];
    };
    static_assert(sizeof(IndirectBlock) == UWUFS_BLOCK_SIZE, "IndirectBlock size must be equal to block size");

    static constexpr uwufs_blk_t LEVEL_0_BLOCKS = UWUFS_DIRECT_BLOCKS;
    static constexpr uwufs_blk_t LEVEL_1_BLOCKS = LEVEL_0_BLOCKS + UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);
    static constexpr uwufs_blk_t LEVEL_2_BLOCKS = LEVEL_1_BLOCKS + UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t) * UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);
    static constexpr uwufs_blk_t LEVEL_3_BLOCKS = LEVEL_2_BLOCKS + UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t) * UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t) * UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);

    INode(uwufs_inode* inode, int device_fd) : inode(inode), device_fd(device_fd) {}

    // auxilliary functions
    bool is_used() const { return inode->file_mode ^ F_TYPE_FREE; }
    bool is_reg() const { return inode->file_mode & F_TYPE_REGULAR; }
    bool is_dir() const { return inode->file_mode & F_TYPE_DIRECTORY; }

    // returns the block number of index-th data block
    // no bounds checking: index
    uwufs_blk_t get_dblk(uwufs_blk_t index) const;

    // iterators
    // no bounds checking: start_index
    DataBlockIterator dblk_itr(uwufs_blk_t start_index = 0) const;

    uwufs_inode* inode; // not owned
    int device_fd;

    // static functions
    static uwufs_blk_t static_get_dblk(const uwufs_inode* inode, int device_fd, uwufs_blk_t index);
    static DataBlockIterator static_dblk_itr(const uwufs_inode* inode, int device_fd, uwufs_blk_t start_index);
};


#endif