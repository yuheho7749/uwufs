#include "c_api.h"

#include "INode.h"
#include "DataBlockIterator.h"


uwufs_blk_t get_dblk(const uwufs_inode* inode, int device_fd, uwufs_blk_t index) {
    return INode::static_get_dblk(inode, device_fd, index);
}

dblk_itr_t create_dblk_itr(const uwufs_inode* inode, int device_fd, uwufs_blk_t start_index) {
    return new DataBlockIterator(inode, device_fd, start_index);
}

uwufs_blk_t dblk_itr_next(dblk_itr_t itr) {
    return static_cast<DataBlockIterator*>(itr)->next();
}

void destroy_dblk_itr(dblk_itr_t itr) {
    delete static_cast<DataBlockIterator*>(itr);
}