#include "FileSystem.h"
#include "Disk.h"
#include "FileSystemPolicy.h"

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
    Ext4Policy::INode inode(1);
    for (std::size_t i{0}; i < 128; ++i) {
        auto ino{policy.alloc_inode(&disk)};
        policy.write_inode(&disk, ino, &inode); // set link_cnt to 1
        std::cout << "inode " << ino << std::endl;
    }
    policy.alloc_inode(&disk);
    return 0;
}