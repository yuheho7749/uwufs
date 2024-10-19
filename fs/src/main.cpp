#include "FileSystem.h"
#include "Disk.h"

#include <cassert>
#include <string>
#include <iostream>

// int main(int argc, char* argv[]) {
//     FileSystem::init(argc, argv);
//     return FileSystem::get()->launch();
// }

int main() {
    VDB vdb;
    std::string text = "Hello, World!";
    vdb.write(0, text.size(), text.c_str());
    char buf[13];
    vdb.read(0, 13, buf);
    std::cout << buf << std::endl;
    return 0;
}