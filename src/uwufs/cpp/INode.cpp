#include "INode.h"
#include "DataBlockIterator.h"

#include "../low_level_operations.h"
#include <cstdio>
#include <cstring>


#ifndef assert_low_level_operation
#define assert_low_level_operation(operation) \
    if (operation < 0) { \
        return 0; \
    }
#endif


uwufs_blk_t INode::get_dblk(uwufs_blk_t index) const {
    // directly build a temporary iterator
    // so it is not recommended to do random access on data blocks
    return dblk_itr(index).next();
}

DataBlockIterator INode::dblk_itr(uwufs_blk_t start_index) const {
    return DataBlockIterator(inode, device_fd, start_index);
}

uwufs_blk_t INode::static_get_dblk(const uwufs_inode* inode, int device_fd, uwufs_blk_t index) {
    // directly build a temporary iterator
    // so it is not recommended to do random access on data blocks
    return DataBlockIterator(inode, device_fd, index).next();
}

DataBlockIterator INode::static_dblk_itr(const uwufs_inode* inode, int device_fd, uwufs_blk_t start_index) {
    return DataBlockIterator(inode, device_fd, start_index);
}

uwufs_blk_t INode::recursive_append_dblk(int device_fd, uint8_t level, uwufs_blk_t cur_no, uwufs_blk_t index, uwufs_blk_t block_no) {
    INode::IndirectBlock indirect_block;
    if (index == 0) {   // need to allocate a new indirect block
        assert_low_level_operation(malloc_blk(device_fd, &cur_no));
        memset(&indirect_block, 0, UWUFS_BLOCK_SIZE);
#ifdef DEBUG
        printf("new indirect block: %lu\n", cur_no);
#endif
    }
    else {
        assert_low_level_operation(read_blk(device_fd, &indirect_block, cur_no));
    }
    if (level == 0) {   // cur_no is the single indirect block
        indirect_block.block_nos[index] = block_no;
        assert_low_level_operation(write_blk(device_fd, &indirect_block, cur_no));
        return cur_no;
    }
    // recursively append the data block
    uwufs_blk_t mod = 1;
    for (uint8_t i{0}; i < level; ++i) {
        mod *= UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);
    }
    indirect_block.block_nos[index / mod] = recursive_append_dblk(device_fd, level - 1, indirect_block.block_nos[index / mod], index % mod, block_no);
    assert_low_level_operation(write_blk(device_fd, &indirect_block, cur_no));  // write back the indirect block
    return cur_no;
}

uwufs_blk_t INode::append_dblk(uwufs_inode* inode, int device_fd, uwufs_blk_t index, uwufs_blk_t block_no) {
    // It will write all modification directly to the disk EXCEPT the inode itself.
    // Remember to write the inode to disk after calling this function.
    // It assumes `index` is the position of the new data block no.
    // index == current block count
    if (index >= LEVEL_3_BLOCKS) {
        return 0;
    }
    if (index < LEVEL_0_BLOCKS) {   // direct block
        inode->direct_blks[index] = block_no;
    }
    else if (index < LEVEL_1_BLOCKS) {  // single indirect block
        inode->single_indirect_blks = recursive_append_dblk(device_fd, 0, inode->single_indirect_blks, index - LEVEL_0_BLOCKS, block_no);
    }
    else if (index < LEVEL_2_BLOCKS) {  // double indirect block
        inode->double_indirect_blks = recursive_append_dblk(device_fd, 1, inode->double_indirect_blks, index - LEVEL_1_BLOCKS, block_no);
    }
    else {  // triple indirect block
        inode->triple_indirect_blks = recursive_append_dblk(device_fd, 2, inode->triple_indirect_blks, index - LEVEL_2_BLOCKS, block_no);
    }
    return block_no;
}

std::pair<uwufs_blk_t, bool> INode::recursive_remove_dblk(int device_fd, uint8_t level, uwufs_blk_t cur_no, uwufs_blk_t index) {
    // Returns true if the current indirect block is empty after removing the data block.
    INode::IndirectBlock indirect_block;
    if (read_blk(device_fd, &indirect_block, cur_no) < 0) {
        return {0, false};
    }
    if (level == 0) {   // cur_no is the single indirect block
        uwufs_blk_t block_no = indirect_block.block_nos[index];
        if (index == 0) {
            if (free_blk(device_fd, cur_no) < 0) {
                return {0, true};
            }
#ifdef DEBUG
            printf("free indirect block: %lu\n", cur_no);
#endif
            return {block_no, true};
        }
        indirect_block.block_nos[index] = 0;
        if (write_blk(device_fd, &indirect_block, cur_no) < 0) {
            return {0, false};
        }
        return {block_no, false};
    }
    uwufs_blk_t mod = 1;
    for (uint8_t i{0}; i < level; ++i) {
        mod *= UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);
    }
    auto [block_no, free] = recursive_remove_dblk(device_fd, level - 1, indirect_block.block_nos[index / mod], index % mod);
    if (free) { // the child indirect block is empty
        if (index == 0) {   // need to free the current indirect block
            if (free_blk(device_fd, cur_no) < 0) {
                return {0, true};
            }
#ifdef DEBUG
            printf("free indirect block: %lu\n", cur_no);
#endif
            return {block_no, true};
        }
        indirect_block.block_nos[index / mod] = 0;
        if (write_blk(device_fd, &indirect_block, cur_no) < 0) {
            return {0, false};
        }
    }
    return {block_no, false};
}

uwufs_blk_t INode::remove_dblk(uwufs_inode *inode, int device_fd, uwufs_blk_t index) {
    // It will write all modification directly to the disk EXCEPT the inode itself.
    // Remember to write the inode to disk after calling this function.
    // It assumes `index` is the position of the data block to be removed.
    // Returns the block number of the removed data block.
    // It will not free the data block.
    if (index >= LEVEL_3_BLOCKS) {
        return 0;
    }
    if (index < LEVEL_0_BLOCKS) {   // direct block
        auto block_no = inode->direct_blks[index];
        inode->direct_blks[index] = 0;
        return block_no;
    }
    if (index < LEVEL_1_BLOCKS) {  // single indirect block
        auto [block_no, free] = recursive_remove_dblk(device_fd, 0, inode->single_indirect_blks, index - LEVEL_0_BLOCKS);
        if (free) {
            inode->single_indirect_blks = 0;
        }
        return block_no;
    }
    if (index < LEVEL_2_BLOCKS) {  // double indirect block
        auto [block_no, free] = recursive_remove_dblk(device_fd, 1, inode->double_indirect_blks, index - LEVEL_1_BLOCKS);
        if (free) {
            inode->double_indirect_blks = 0;
        }
        return block_no;
    }
    // triple indirect block
    auto [block_no, free] = recursive_remove_dblk(device_fd, 2, inode->triple_indirect_blks, index - LEVEL_2_BLOCKS);
    if (free) {
        inode->triple_indirect_blks = 0;
    }
    return block_no;
}

void INode::remove_dblks(uwufs_inode *inode, int device_fd, uwufs_blk_t start_index, uwufs_blk_t end_index) {
    // It will write all modification directly to the disk EXCEPT the inode itself.
    // Remember to write the inode to disk after calling this function.
    // Returns the block number of the removed data block.
    // It will not free the data block.
    // free the data block numbers: [start_index, end_index)
    // for (uwufs_blk_t i{start_index}; i < end_index; ++i) {
    //     remove_dblk(inode, device_fd, i);
    // }
    if (start_index >= end_index) {
        return;
    }
    recursive_remove_dblks(device_fd, inode->single_indirect_blks, LEVEL_0_BLOCKS, LEVEL_1_BLOCKS, start_index, end_index);
    recursive_remove_dblks(device_fd, inode->double_indirect_blks, LEVEL_1_BLOCKS, LEVEL_2_BLOCKS, start_index, end_index);
    recursive_remove_dblks(device_fd, inode->triple_indirect_blks, LEVEL_2_BLOCKS, LEVEL_3_BLOCKS, start_index, end_index);
}

void INode::recursive_remove_dblks(int device_fd, uwufs_blk_t cur_no, uwufs_blk_t cur_left, uwufs_blk_t cur_right, uwufs_blk_t start_index, uwufs_blk_t end_index) {
    // free the data block numbers: [start_index, end_index)
    // the current indirect block contains the data blocks: [cur_left, cur_right)
    if (start_index >= cur_right || end_index <= cur_left) {    // no overlap
        return;
    }
    auto stride{(cur_right - cur_left) / (UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t))};
    if (stride > 1) {   // not single indirect block
        INode::IndirectBlock indirect_block;
        if (read_blk(device_fd, &indirect_block, cur_no) < 0) {
#ifdef DEBUG
            printf("failed to read indirect block: %lu\n", cur_no);
#endif
            return;
        }
        for (uwufs_blk_t i{0}; i < UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t); ++i) {
            recursive_remove_dblks(device_fd, indirect_block.block_nos[i], cur_left + i * stride, cur_left + (i + 1) * stride, start_index, end_index);
        }
    }
    if (start_index <= cur_left) {  // need to free the current indirect block
#ifdef DEBUG
        printf("free indirect block: %lu\n", cur_no);
#endif
        if (free_blk(device_fd, cur_no) < 0) {
#ifdef DEBUG
            printf("failed to free indirect block: %lu\n", cur_no);
#endif
        }
    }
}