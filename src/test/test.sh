#!/usr/bin/env bash

cd /mnt/uwufs

# for i in {1..36}
# do
#   mkdir "dir_$i"
# done

for i in {1..24}
do
  rmdir "dir_$i"
  echo "Removed: dir_$i"
done

# for dir in */
# do
#   # Check if it's a directory
#   if [ -d "$dir" ]; then
#     rmdir "$dir"
#     echo "Removed: $dir"
#   fi
# done
