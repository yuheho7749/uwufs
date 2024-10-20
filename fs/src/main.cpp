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
    std::cout << "mkfs done" << std::endl;
    return 0;
}