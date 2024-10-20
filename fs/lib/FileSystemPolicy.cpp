#include "FileSystemPolicy.h"
#include "Disk.h"
#include <bits/c++config.h>


template <typename DerivedDisk>
void Ext4Policy::mkfs_impl(DerivedDisk *disk, std::size_t ninodes, std::size_t nblocks) {
    // create a superblock
    // we implement Linux SysV free list
    SuperBlock sb;
    sb.ninodes = ninodes;
    sb.niblocks = (ninodes * ext4::INODE_SIZE + ext4::BLOCK_SIZE - 1) / ext4::BLOCK_SIZE;
    nblocks -= sb.niblocks + 1;
    if (nblocks < 0) {
        throw NotEnoughBlocksError();
    }
    
    // and more

    // write superblock to disk
    disk->write(0, sizeof(SuperBlock), &sb);

    // create inodes in disk

    // create data blocks in disk
}