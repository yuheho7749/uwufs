#ifndef Disk_H
#define Disk_H

#include <cstring>
#include <unistd.h>


// CRTP base class
// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
template <typename Derived>
class Disk {
public:
    struct DiskOpenError {};

    // read len bytes from the disk starting at pos into buf
    // unsafe: no bounds checking
    ssize_t read(std::size_t pos, std::size_t len, void* buf) noexcept {
        return static_cast<Derived*>(this)->read_impl(pos, len, buf);
    }

    // write len bytes from buf to the disk starting at pos
    // unsafe: no bounds checking
    ssize_t write(std::size_t pos, std::size_t len, const void* buf) noexcept {
        return static_cast<Derived*>(this)->write_impl(pos, len, buf);
    }
};


// Disk implementation using a fixed-size array
// for debugging purposes
template <std::size_t MAX_SIZE>
class StringDisk : public Disk<StringDisk<MAX_SIZE>> {
public:
    ssize_t read_impl(std::size_t pos, std::size_t len, void* buf) noexcept {
        std::memcpy(buf, data + pos, len);
        return len;
    }

    ssize_t write_impl(std::size_t pos, std::size_t len, const void* buf) noexcept {
        std::memcpy(data + pos, buf, len);
        return len;
    }

private:
    char data[MAX_SIZE];
};


class VDB : public Disk<VDB> {
public:
    static constexpr std::size_t SECTOR_SIZE = 512;
    static constexpr std::size_t N_SECTORS = 104857600;
    static constexpr char VDB_PATH[] = "/dev/vdb";

    VDB();  // throws DiskOpenError

    ~VDB() noexcept {
        close(fd);
    }

    ssize_t read_impl(std::size_t pos, std::size_t len, void* buf) noexcept;
    ssize_t write_impl(std::size_t pos, std::size_t len, const void* buf) noexcept;

private:
    int fd;
};

#endif