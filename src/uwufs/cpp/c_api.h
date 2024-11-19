#ifndef c_api_h
#define c_api_h

#ifdef __cplusplus
extern "C" {
#endif

#define dblk_itr_t void*

#include "../uwufs.h"

/**
 * Returns the block number of index-th data block of the inode.
 * No bounds checking: index
 */
uwufs_blk_t get_dblk(const struct uwufs_inode* inode, int device_fd, uwufs_blk_t index);

/**
 * Creates a new data block iterator for the inode.
 * No bounds checking: start_index
 * Caveat: This iterator will be invalidated if the inode's data block number entries are changed (not the data blocks themselves are modified).
 */
dblk_itr_t create_dblk_itr(const struct uwufs_inode* inode, int device_fd, uwufs_blk_t start_index);

/**
 * Returns the block number of the next data block.
 * Caveat: You cannot check out of bounds by checking if the return value is 0.
 * Because empty entries are not guaranteed to be set to 0.
 * Correct way to iterate:
    * ```
    * for (uwufs_blk_t index = 0; index < inode->file_size / UWUFS_BLOCK_SIZE; ++index) {
    *     uwufs_blk_t blk_no = dblk_itr_next(itr);
    *     // do something with blk_no
    * }
    * ```
 * NOT:
    * ```
    * for (uwufs_blk_t blk_no = dblk_itr_next(itr); blk_no != 0; blk_no = dblk_itr_next(itr)) {
    *     // do something with blk_no
    * }
    * ```
 */
uwufs_blk_t dblk_itr_next(dblk_itr_t itr);

/**
 * Destroys the data block iterator.
 * Caveat: Do not call free() on it.
 */
void destroy_dblk_itr(dblk_itr_t itr);

#ifdef __cplusplus
}
#endif


#endif