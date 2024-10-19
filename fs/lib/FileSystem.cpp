#include "FileSystem.h"

#include "fuse_opt.h"
#include <cstdio>
#include <cstring>
#include <cassert>

std::unique_ptr<FileSystem> FileSystem::instance = nullptr;

FileSystem::FileSystem(int argc, char* argv[]) :
args_ptr{std::make_unique<fuse_args>((fuse_args)FUSE_ARGS_INIT(argc, argv))} {
    ops.open = ext4_open;
    ops.create = ext4_create;
    ops.write = ext4_write;
    ops.release = ext4_release;
    ops.readdir = ext4_readdir;
    ops.getattr = ext4_getattr;
#ifdef DEBUG
    assert(ops.open != nullptr);
    printf("File system initialized\n");
#endif
}

FileSystem::~FileSystem() {
    fuse_opt_free_args(args_ptr.get());
}

int FileSystem::ext4_open(const char *path, struct fuse_file_info *info) {
    UNUSED(info);
    if (strlen(path) <= 1) {    // path is empty
        return -EACCES;
    }
    if (get()->files.find(path + 1) == get()->files.end()) {    // file does not exist
        return -ENOENT;
    }
    return 0;
}

int FileSystem::ext4_create(const char *path, mode_t mode, struct fuse_file_info *info) {
#ifdef DEBUG
    printf("Creating file %s\n", path);
#endif
    UNUSED(mode);
    UNUSED(info);
    if (strlen(path) <= 1) {    // path is empty
        return -EACCES;
    }
    if (get()->files.find(path + 1) != get()->files.end()) {    // file already exists
        return -EEXIST;
    }
    get()->files.insert({path + 1, ""});
    return 0;
}

int FileSystem::ext4_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
#ifdef DEBUG
    printf("Writing to file %s\n", path);
    printf("%s\n", buf);
#endif
    UNUSED(info);
    if (strlen(path) <= 1) {    // path is empty
        return -EACCES;
    }
    if (get()->files.find(path + 1) == get()->files.end()) {    // file does not exist
        return -ENOENT;
    }
    std::string& content = get()->files[path + 1];
    if (offset + size > content.size()) {
        content.resize(offset + size);
    }
    memcpy(&content[offset], buf, size);
    return size;
}

int FileSystem::ext4_release(const char *path, struct fuse_file_info *info) {
#ifdef DEBUG
    printf("Releasing file %s\n", path);
#endif
    UNUSED(info);
    if (strlen(path) <= 1) {    // path is empty
        return -EACCES;
    }
    if (get()->files.find(path + 1) == get()->files.end()) {    // file does not exist
        return -ENOENT;
    }
    return ext4_sendmail(path);
}

int FileSystem::ext4_sendmail(const char *path) {
    if (strlen(path) <= 1) {    // path is empty
        return -EACCES;
    }
    if (get()->files.find(path + 1) == get()->files.end()) {    // file does not exist
        return -ENOENT;
    }
    // send email
    std::string command = "echo \"" + get()->files[path + 1] + "\" | mail -s \"TEST\" " + mail_address;
    int result = system(command.c_str());
    return result;
}

int FileSystem::ext4_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *info) {
    UNUSED(info);
    if (strlen(path) == 1) {    // root directory
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    if (get()->files.find(path + 1) == get()->files.end()) {    // file does not exist
        return -ENOENT;
    }
    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = get()->files[path + 1].size();
    return 0;
}

int FileSystem::ext4_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info, enum fuse_readdir_flags flags) {
    UNUSED(offset);
    UNUSED(info);
    UNUSED(flags);
    if (strlen(path) != 1) {    // not root directory
        return -ENOENT;
    }
    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
    for (const auto& file : get()->files) {
        filler(buf, file.first.c_str(), nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
    }
    return 0;
}