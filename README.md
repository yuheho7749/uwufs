# Universal Working Userspace File System (UWUFS)
A universal and (hopefully) working userspace filesystem for 64-bit linux-like systems.

# Instructions

## Phase1: Send email with fuse
1. Run `make phase1` command to build binaries.
2. Create a file named `email.txt` with an email address you wish to send email in the same directory as the `phase1` binary.
3. Start and mount the fuse filesystem with `./phase1 [your/mnt/point/] [optional: flags]`. You can optionally include the `-f` flag to make fuse run in the foreground.
4. Writing to any file in the mounted fuse filesystem will send an email.

## Phase2: A working uwufs
TBD
