# Universal Working Userspace File System (UWUFS)
A universal and (hopefully) working fuse userspace filesystem for unix/linux-like systems.

# Instructions

## Phase1: Send email with fuse
1. Run `make phase1` command to build binaries.
2. Start and mount the fuse filesystem with `./phase1 [your/mnt/point/] [optional: flags]`. You can optionally include the `-f` flag to make fuse run in the foreground.
3. Writing to any file in the mounted fuse filesystem will send an email.

## Phase2: A working uwufs
TBD
