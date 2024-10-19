#ifndef FileSystemPolicy_H
#define FileSystemPolicy_H

#include <cstddef>

// CRTP base class
// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// Policy does not own the disk
template <typename Derived>
class FileSystemPolicy {
public:
    template <typename DerivedDisk>
    void mkfs(DerivedDisk* disk, std::size_t ninodes, std::size_t ndblocks) {
        static_cast<Derived*>(this)->mkfs_impl(disk, ninodes, ndblocks);
    }

    template <typename DerivedDisk>
    void alloc_inode(DerivedDisk* disk) {
        static_cast<Derived*>(this)->alloc_inode_impl();
    }

    template <typename DerivedDisk>
    void alloc_block(DerivedDisk* disk) {
        static_cast<Derived*>(this)->alloc_block_impl();
    }

    template <typename DerivedDisk>
    void free_inode(DerivedDisk* disk) {
        static_cast<Derived*>(this)->free_inode_impl();
    }

    template <typename DerivedDisk>
    void free_block(DerivedDisk* disk) {
        static_cast<Derived*>(this)->free_block_impl();
    }

    template <typename DerivedDisk>
    void read_inode(DerivedDisk* disk) {
        static_cast<Derived*>(this)->read_inode_impl();
    }

    template <typename DerivedDisk>
    void write_inode(DerivedDisk* disk) {
        static_cast<Derived*>(this)->write_inode_impl();
    }

    template <typename DerivedDisk>
    void read_block(DerivedDisk* disk) {
        static_cast<Derived*>(this)->read_block_impl();
    }

    template <typename DerivedDisk>
    void write_block(DerivedDisk* disk) {
        static_cast<Derived*>(this)->write_block_impl();
    }
};


class Ext4Policy : public FileSystemPolicy<Ext4Policy> {
// actually this is not implemented as ext4 but share the same specifications
public:
    // core data structures
    struct SuperBlock {
        std::size_t ninodes;
        std::size_t ndblocks;
        std::size_t nfree_inodes;
        std::size_t nfree_dblocks;
        std::size_t first_free_dblock;
    };

    struct INode {
        unsigned allocated : 1;
    };

    // all metadata is stored in the superblock
    // there are two types of metadata:
    // 1. static metadata: known at compile time, in ext4 struct
    // 2. dynamic metadata: should be specified by mkfs and stored in the superblock
    Ext4Policy() noexcept = default;
    ~Ext4Policy() noexcept = default;

    // ninodes: number of inodes
    // ndblocks: number of data blocks
    template <typename DerivedDisk>
    void mkfs_impl(DerivedDisk* disk, std::size_t ninodes, std::size_t ndblocks);
    void alloc_inode_impl();
};


#endif