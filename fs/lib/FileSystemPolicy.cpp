#include "FileSystemPolicy.h"
#include "Disk.h"


template <typename DerivedDisk>
void Ext4Policy::mkfs_impl(DerivedDisk *disk, std::size_t ninodes, std::size_t ndblocks) {
    // create a superblock in disk
    SuperBlock sb;
    sb.ninodes = ninodes;
    sb.ndblocks = ndblocks;
    sb.nfree_inodes = ninodes;
    sb.nfree_dblocks = ndblocks;
    // and more
    disk->write(0, sizeof(SuperBlock), &sb);

    // create inodes in disk

    // create data blocks in disk
}