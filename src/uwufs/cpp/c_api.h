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

/**
 * Appends a new data block to the inode.
 * It will write all modification directly to the disk EXCEPT the inode itself (but will modify the struct inode in memory).
 * Remember to write the inode to disk after calling this function.
 * It assumes `index` is the position of the new data block no.
 * You need to make sure `index` == the last data block index + 1.
 * Returns the block number of the new data block.
 * Returns 0 if the operation fails.
 */
uwufs_blk_t append_dblk(struct uwufs_inode* inode, int device_fd, uwufs_blk_t index, uwufs_blk_t block_no);

/**
 * It will write all modification directly to the disk EXCEPT the inode itself (but will modify the struct inode in memory).
 * It assumes `index` is the position of the data block to be removed.
 * You need to make sure `index` == the last data block index.
 * Returns the block number of the removed data block.
 */
uwufs_blk_t remove_dblk(struct uwufs_inode* inode, int device_fd, uwufs_blk_t index);

/**
 * It will write all modification directly to the disk EXCEPT the inode itself (but will modify the struct inode in memory).
 * It will not free the data block.
 * free the data block numbers: [start_index, end_index)
 * equivalent to:
   * ```
   * for (uwufs_blk_t i{start_index}; i < end_index; ++i) {
   *     remove_dblk(inode, device_fd, i);
   * }
 */
void remove_dblks(struct uwufs_inode* inode, int device_fd, uwufs_blk_t start_index, uwufs_blk_t end_index);

#ifdef __cplusplus
}
#endif


#endif
