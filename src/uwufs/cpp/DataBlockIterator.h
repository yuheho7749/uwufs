#ifndef DataBlockIterator_h
#define DataBlockIterator_h

#include "../uwufs.h"
#include "INode.h"
#include <bits/stdint-uintn.h>
#include <iterator>
#include <memory>


// Auxiliary classes for iterating over indirect blocks
class IndirectBlockIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = uwufs_blk_t; // block no

    IndirectBlockIterator(uwufs_blk_t iblk_no, int device_fd, const uwufs_inode* inode, uwufs_blk_t start_index = 0);
    virtual ~IndirectBlockIterator() = default;

    // returns the current block no (returns 0 if no more blocks)
    // advances the iterator
    // no bounds checking
    virtual value_type next() = 0;

    // returns a new iterator for the next indirect block
    // returns nullptr if no more indirect blocks
    virtual std::unique_ptr<IndirectBlockIterator> next_itr() = 0;

protected:
    std::unique_ptr<INode::IndirectBlock> iblk;
    int device_fd;
    const uwufs_inode* inode; // not owned
    uwufs_blk_t current_index;
};

class SingleIndirectBlockIterator : public IndirectBlockIterator {
public:
    SingleIndirectBlockIterator(uwufs_blk_t iblk_no, int device_fd, const uwufs_inode* inode, uwufs_blk_t start_i) : IndirectBlockIterator(iblk_no, device_fd, inode, start_i) {}
    value_type next() override;
    std::unique_ptr<IndirectBlockIterator> next_itr() override;
};

class DoubleIndirectBlockIterator : public IndirectBlockIterator {
public:
    DoubleIndirectBlockIterator(uwufs_blk_t iblk_no, int device_fd, const uwufs_inode* inode, uwufs_blk_t start_i, uwufs_blk_t start_j);
    value_type next() override;
    std::unique_ptr<IndirectBlockIterator> next_itr() override;

private:
    std::unique_ptr<SingleIndirectBlockIterator> single_itr;
};

class TripleIndirectBlockIterator : public IndirectBlockIterator {
public:
    TripleIndirectBlockIterator(uwufs_blk_t iblk_no, int device_fd, const uwufs_inode* inode, uwufs_blk_t start_i, uwufs_blk_t start_j, uwufs_blk_t start_k);
    value_type next() override;
    std::unique_ptr<IndirectBlockIterator> next_itr() override;

private:
    std::unique_ptr<DoubleIndirectBlockIterator> double_itr;
};


// This iterator provides a forward iterator for iterating over data blocks of an inode
// Lazy Loading: Reads the blocks at most once only when needed
// Good for iterating over consecutive blocks
// Will be invalidated if the inode is modified
class DataBlockIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = uwufs_blk_t; // block no

    // no bounds checking: assumes start_index is valid
    DataBlockIterator(const uwufs_inode* inode, int device_fd, uwufs_blk_t start_index);

    // returns the current block no (returns 0 if no more blocks)
    // advances the iterator
    // no bounds checking
    value_type next();

private:
    const uwufs_inode* inode; // not owned
    int device_fd;
    uint8_t current_index;
    std::unique_ptr<IndirectBlockIterator> itr;
};


#endif