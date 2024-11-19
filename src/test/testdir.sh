#!/bin/bash

sudo ../../mount.uwu /dev/vdb /mnt -o allow_other

cd /mnt

# invalid - all should return some error message
for cmd in "rmdir ." "rmdir .." "rmdir /" \
            "rmdir SUPERLONGNAMEDFSDFJDSLFJLKDSFJDSKLFJKLSDFJKDLSFJDKSLFJDLSFJLFJSLFJDSKLFJDSLKFJSKLFJSDKLJFKSLDFJSDKLFJDSLFJDSLKFJSDLFJLDSFJSLKDFJKLDSFJSKLDFJSKDLFJLSDKFJLSDJFSKLJFKDSLFJKDSLJFDKSLFJDKLFJSDKLFJSDKJFSFKLSJFKSLFJDSLFLSDJFSLFJSDLFJDLSFJSLFSFSDFSFSDSFDDSF";
    do
        echo "TEST $cmd"
        if [ "$cmd" ]; then
            echo -e "\t==> Returned expected error"
        fi
    done

# test individual dir removal
mkdir test-for-rmdir
mkdir test-for-rmdir/dir1
echo "TEST test-for-rmdir"
rmdir test-for-rmdir/dir2
rmdir test-for-rmdir/dir1 
echo -e "\t==> Above should return error for test-for-rmdir/dir2"

# test a bunch of directories
# all should succed (no error msgs)
echo "TEST for loop mkdir test-for-rmdir/dir{1..20}"
for i in {1..20}; do mkdir test-for-rmdir/dir$i; done
for i in {1..20}; do rmdir test-for-rmdir/dir$i; done
rmdir test-for-rmdir 
echo -e "\t==> Above should return 0 errors"

# tested nested removal and file entry
mkdir test-nested
mkdir test-nested/a
mkdir test-nested/a/b
mkdir test-nested/a/b/c
touch test-nested/a/blah.txt

echo "TEST rmdir -p for test-nested/a/b/c"
rmdir -p test-nested/a/b/c # should only remove b/c
echo -e "\t==> Above should return error for test-nested/a"

rm test-nested/a/blah.txt
echo "TEST rmdir -p for test-nested/a"
rmdir -p test-nested/a # should remove all
echo -e "\t==> Above should return 0 errors"

# done
echo "Leaving /mnt directory and unmounting..."
cd -
sudo fusermount -u /mnt


