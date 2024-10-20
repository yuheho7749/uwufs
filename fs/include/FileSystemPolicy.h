#ifndef FileSystemPolicy_H
#define FileSystemPolicy_H

#include "defs.h"

#include <bits/c++config.h>
#include <cstddef>

// CRTP base class
// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// Policy does not own the disk
template <typename Derived>
class FileSystemPolicy {
public:
    // customized exceptions
    struct NotEnoughBlocksError {};

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

    // this struct is used to cast the raw data to a block
    // e.g., Block* block = reinterpret_cast<Block*>(data);
    struct alignas(ext4::BLOCK_SIZE) Block {};

    struct SuperBlock {
        std::size_t ninodes;    // number of inodes
        std::size_t niblocks;   // number of inode blocks (ilist)
        std::size_t ndblocks;   // number of data blocks
        std::size_t nfree_inodes;   // number of free inodes
        std::size_t nfree_dblocks;  // number of free data blocks
        std::size_t first_free_list_block;   // first block of the free list
        
    };
    static_assert(sizeof(SuperBlock) <= ext4::BLOCK_SIZE, "size of SuperBlock exceeds block size");

    struct alignas(ext4::INODE_SIZE) INode {
        unsigned allocated : 1;
    };
    static_assert(sizeof(INode) == ext4::INODE_SIZE, "size of INode does not match inode size");

    // all metadata is stored in the superblock
    // there are two types of metadata:
    // 1. static metadata: known at compile time, in ext4 struct
    // 2. dynamic metadata: should be specified by mkfs and stored in the superblock
    Ext4Policy() noexcept = default;
    ~Ext4Policy() noexcept = default;

    // ninodes: number of inodes
    // nblocks: total number of blocks
    template <typename DerivedDisk>
    void mkfs_impl(DerivedDisk* disk, std::size_t ninodes, std::size_t nblocks);
    void alloc_inode_impl();
};


#endif