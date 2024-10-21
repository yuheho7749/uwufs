#ifndef FileSystemPolicy_H
#define FileSystemPolicy_H

#include "defs.h"

#include <algorithm>
#include <cstddef>
#include <memory>

// CRTP base class
// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// Policy does not own the disk
template <typename Derived>
class FileSystemPolicy {
public:
    // customized exceptions
    struct NotEnoughBlocksError {};
    struct NotEnoughInodesError {};

    template <typename DerivedDisk>
    void mkfs(DerivedDisk* disk, std::size_t ninodes, std::size_t nblocks) {
        // throw NotEnoughBlocksError
        static_cast<Derived*>(this)->mkfs_impl(disk, ninodes, nblocks);
    }

    /* INode operations */
    template <typename DerivedDisk>
    std::size_t alloc_inode(DerivedDisk* disk) {
        // return the inode number
        // throw NotEnoughInodesError
        // this function only returns the inode number, the inode is not initialized
        // there's no free_inode(): just set link_cnt to 0
        return static_cast<Derived*>(this)->alloc_inode_impl(disk);
    }

    template <typename DerivedDisk>
    void read_inode(DerivedDisk* disk, std::size_t ino, void* inode) noexcept {
        // unsafe: no bounds checking
        static_cast<Derived*>(this)->read_inode_impl(disk, ino, inode);
    }

    template <typename DerivedDisk, typename T>
    void read_inode_field(DerivedDisk* disk, std::size_t ino, std::size_t field_offset, T& field) noexcept {
        // unsafe: no bounds checking
        static_cast<Derived*>(this)->read_inode_field_impl(disk, ino, field_offset, field);
    }

    template <typename DerivedDisk>
    void write_inode(DerivedDisk* disk, std::size_t ino, const void* inode) noexcept {
        // unsafe: no bounds checking
        static_cast<Derived*>(this)->write_inode_impl(disk, ino, inode);
    }

    template <typename DerivedDisk, typename T>
    void write_inode_field(DerivedDisk* disk, std::size_t ino, std::size_t field_offset, const T& field) noexcept {
        // unsafe: no bounds checking
        static_cast<Derived*>(this)->write_inode_field_impl(disk, ino, field_offset, field);
    }

    /* Data Block operations */
    template <typename DerivedDisk>
    std::size_t alloc_dblock(DerivedDisk* disk) {
        // throw NotEnoughBlocksError
        // allocate a data block
        return static_cast<Derived*>(this)->alloc_dblock_impl(disk);
    }

    template <typename DerivedDisk>
    void free_dblock(DerivedDisk* disk) {
        static_cast<Derived*>(this)->free_dblock_impl(disk);
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
        std::size_t first_free_list_block;   // first block of the free list
    };
    static_assert(sizeof(SuperBlock) <= ext4::BLOCK_SIZE, "size of SuperBlock exceeds block size");

    struct alignas(ext4::INODE_SIZE) INode {
        std::size_t link_cnt;   // number of hard links
        // no need to have allocated flag: if link_cnt == 0, the inode is free
        INode() noexcept = default;
        INode(std::size_t link_cnt) : link_cnt(link_cnt) {}
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

    template <typename DerivedDisk>
    std::size_t alloc_inode_impl(DerivedDisk* disk);

    template <typename DerivedDisk>
    void read_inode_impl(DerivedDisk* disk, std::size_t ino, void* inode) noexcept;

    template <typename DerivedDisk, typename T>
    void read_inode_field_impl(DerivedDisk* disk, std::size_t ino, std::size_t field_offset, T& field) noexcept;

    template <typename DerivedDisk>
    void write_inode_impl(DerivedDisk* disk, std::size_t ino, const void* inode) noexcept;

    template <typename DerivedDisk, typename T>
    void write_inode_field_impl(DerivedDisk* disk, std::size_t ino, std::size_t field_offset, const T& field) noexcept;

    template <typename DerivedDisk>
    std::size_t alloc_dblock_impl(DerivedDisk* disk);

private:
    template <typename DerivedDisk>
    std::size_t create_free_list_block(DerivedDisk* disk, std::size_t curno, std::size_t nblocks);
};

template <typename DerivedDisk>
void Ext4Policy::mkfs_impl(DerivedDisk* disk, std::size_t ninodes, std::size_t nblocks) {
    // the actual ninodes >= specified ninodes (ceil to block size)
    // nblocks = 1 (superblock) + niblocks + ndblocks

    // create a superblock
    // we implement Linux SysV free list
    SuperBlock sb;
    sb.niblocks = (ninodes * ext4::INODE_SIZE + ext4::BLOCK_SIZE - 1) / ext4::BLOCK_SIZE;
    sb.ninodes = sb.niblocks * ext4::BLOCK_SIZE / ext4::INODE_SIZE;
    if (nblocks < sb.niblocks + 2) {    // 1 for superblock, 1 for free list head
        throw NotEnoughBlocksError();
    }
    sb.ndblocks = nblocks - sb.niblocks - 1;

    // create inodes in disk (directly write to disk)
    auto offset{ext4::BLOCK_SIZE};
    INode inode(0);
    for (std::size_t i{0}; i < sb.ninodes; ++i) {
        disk->write(offset, ext4::INODE_SIZE, &inode);
        offset += ext4::INODE_SIZE;
    }

    // create a free list in disk (directly write to disk)
    auto curno{sb.niblocks + 1};
    sb.first_free_list_block = curno;
    while (curno) {
        curno = create_free_list_block(disk, curno, nblocks);
    }
    // write superblock to disk
    disk->write(0, sizeof(SuperBlock), &sb);
}

template <typename DerivedDisk>
size_t Ext4Policy::create_free_list_block(DerivedDisk* disk, std::size_t curno, std::size_t nblocks) {
    // return the next free list block number (0 if no more blocks)
    auto offset{curno * ext4::BLOCK_SIZE + sizeof(std::size_t)};
    auto nextno{curno + ext4::BLOCK_SIZE / sizeof(std::size_t)};
    for (std::size_t i{curno + 1}; i < std::min(nextno, nblocks); ++i) {
        disk->write(offset, sizeof(std::size_t), &i);
        offset += sizeof(std::size_t);
    }
    nextno = (nextno < nblocks) ? nextno : 0;
    disk->write(curno * ext4::BLOCK_SIZE, sizeof(std::size_t), &nextno);
    return nextno;
}

template <typename DerivedDisk>
std::size_t Ext4Policy::alloc_inode_impl(DerivedDisk* disk) {
    // return the inode number
    // throw NotEnoughInodesError
    std::size_t ninodes;
    disk->read(offsetof(SuperBlock, ninodes), sizeof(std::size_t), &ninodes);
    std::size_t link_cnt;
    for (std::size_t i{0}; i < ninodes; ++i) {
        read_inode_field(disk, i, offsetof(INode, link_cnt), link_cnt);
        if (link_cnt == 0) {
            return i;
        }
    }
    throw NotEnoughInodesError();
}

template <typename DerivedDisk>
void Ext4Policy::read_inode_impl(DerivedDisk* disk, std::size_t ino, void* inode) noexcept {
    disk->read(ext4::BLOCK_SIZE + ino * ext4::INODE_SIZE, ext4::INODE_SIZE, inode);
}

template <typename DerivedDisk, typename T>
void Ext4Policy::read_inode_field_impl(DerivedDisk* disk, std::size_t ino, std::size_t field_offset, T& field) noexcept {
    disk->read(ext4::BLOCK_SIZE + ino * ext4::INODE_SIZE + field_offset, sizeof(T), &field);
}

template <typename DerivedDisk>
void Ext4Policy::write_inode_impl(DerivedDisk* disk, std::size_t ino, const void* inode) noexcept {
    disk->write(ext4::BLOCK_SIZE + ino * ext4::INODE_SIZE, ext4::INODE_SIZE, inode);
}

template <typename DerivedDisk, typename T>
void Ext4Policy::write_inode_field_impl(DerivedDisk* disk, std::size_t ino, std::size_t field_offset, const T& field) noexcept {
    disk->write(ext4::BLOCK_SIZE + ino * ext4::INODE_SIZE + field_offset, sizeof(T), &field);
}

template <typename DerivedDisk>
std::size_t Ext4Policy::alloc_dblock_impl(DerivedDisk* disk) {
    // throw NotEnoughBlocksError
    // allocate a data block
    std::size_t headno;
    disk->read(offsetof(SuperBlock, first_free_list_block), sizeof(std::size_t), &headno);
    if (headno == 0) {
        throw NotEnoughBlocksError();
    }
    // find the first non-zero block number (not the first block number)
    std::size_t curno;
    for (std::size_t i{1}; i < ext4::BLOCK_SIZE / sizeof(std::size_t); ++i) {
        disk->read(headno * ext4::BLOCK_SIZE + i * sizeof(std::size_t), sizeof(std::size_t), &curno);
        if (curno) {
            size_t zero{0};
            disk->write(headno * ext4::BLOCK_SIZE + i * sizeof(std::size_t), sizeof(std::size_t), &zero);
            return curno;
        }
    }
    // the block is empty, allocate this block
    std::size_t nextno;
    disk->read(headno * ext4::BLOCK_SIZE, sizeof(std::size_t), &nextno);
    // update the head
    disk->write(offsetof(SuperBlock, first_free_list_block), sizeof(std::size_t), &nextno);
    return headno;
}


#endif