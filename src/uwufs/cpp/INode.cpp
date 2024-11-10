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