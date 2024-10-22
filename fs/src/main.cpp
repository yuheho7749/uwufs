#include "FileSystem.h"
#include "Disk.h"
#include "FileSystemPolicy.h"

#include <vector>
#include <cassert>
#include <string>
#include <iostream>

// int main(int argc, char* argv[]) {
//     FileSystem::init(argc, argv);
//     return FileSystem::get()->launch();
// }

int main() {
    StringDisk<1024 * 1024> disk;
    Ext4Policy policy;
    policy.mkfs(&disk, 128, 1024 * 1024 / 4096);
    std::cout << "mkfs done" << std::endl;
    for (std::size_t i{0}; i < 128; ++i) {
        auto ino{policy.alloc_inode(&disk)};
        policy.write_inode_field(&disk, ino, offsetof(Ext4Policy::INode, link_cnt), 1);
        std::cout << "inode " << ino << std::endl;
    }
    try {
        policy.alloc_inode(&disk);
    } catch (const Ext4Policy::NotEnoughInodesError& e) {
        std::cout << "Not enough inodes" << std::endl;
    }
    std::cout << "alloc inode done" << std::endl;
    std::vector<std::size_t> dbnos;
    while (true) {
        try {
            auto ib{policy.alloc_dblock(&disk)};
            std::cout << "block " << ib << std::endl;
            dbnos.push_back(ib);
        } catch (const Ext4Policy::NotEnoughBlocksError& e) {
            std::cout << "Not enough blocks" << std::endl;
            break;
        }
    }
    std::cout << "alloc dblock done" << std::endl;
    std::cout << "used dblocks: " << dbnos.size() << std::endl;
    for (auto dbno : dbnos) {
        policy.free_dblock(&disk, dbno);
    }
    std::cout << "free dblock done" << std::endl;
    dbnos.clear();
    while (true) {
        try {
            auto ib{policy.alloc_dblock(&disk)};
            std::cout << "block " << ib << std::endl;
            dbnos.push_back(ib);
        } catch (const Ext4Policy::NotEnoughBlocksError& e) {
            std::cout << "Not enough blocks" << std::endl;
            break;
        }
    }
    std::cout << "alloc dblock again done" << std::endl;
    std::cout << "used dblocks: " << dbnos.size() << std::endl;
    return 0;
}