#include "INode.h"
#include "DataBlockIterator.h"


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