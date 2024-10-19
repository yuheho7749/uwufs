#include "Disk.h"

#include <fcntl.h>
#include <cassert>


VDB::VDB() : fd(open(VDB_PATH, O_RDWR | O_CREAT, 0644)) {
    if (fd < 0) {
        throw DiskOpenError();
    }
}

void VDB::read_impl(std::size_t pos, std::size_t len, void* buf) noexcept {
    auto ret{pread(fd, buf, len, pos)};
#ifdef DEBUG
    assert(ret == len);
#endif
}

void VDB::write_impl(std::size_t pos, std::size_t len, const void* buf) noexcept {
    auto ret{pwrite(fd, buf, len, pos)};
#ifdef DEBUG
    assert(ret == len);
#endif
}
