#!/usr/bin/env bash

cd /mnt/uwufs

# for i in {1..10000}
# do
#   mkdir "dir$i"
# done

for i in {1..10000}
do
  rmdir "dir$i"
done

# for dir in */
# do
#   # Check if it's a directory
#   if [ -d "$dir" ]; then
#     rmdir "$dir"
#     echo "Removed: $dir"
#   fi
# done

# for i in {1..500}
# do
#   touch "file$i.txt"
# done
