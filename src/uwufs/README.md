# UWUFS Specifications
*Each block must be in 4k bytes (4096 bytes)*

+---------------------+
| Super block         |
+---------------------+
| Reserved space      |
| for extensibility   |
| (aka journaling?)   |
+---------------------+
| I-Nodes (10%)       |
|                     |
|                     |
|                     |
|                     |
+---------------------+
| Data Blocks         |
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
|                     |
+---------------------+


## Super block
- Total blks
- Ilist start
- Ilist total size
- Freelist start
- Freelist total size
- Freelist head

## Free list
Simple linked list

## I-List/I-Nodes
Default to 10% of total volume


### Directories
Root directory is inode 2

File entry is 256 bytes total (248 for file name + 8 bytes for inode)

### Files
10 direct
1 indirect
1 double indirect
1 triple indirect

file type/permissions
- handle actual checking of permissions later


------------------------------------

## Reserved space (not required)
Only if there is time

### Journaling?
- Reserve inode X to be the journaling file
- Reserve only set constant space for journaling
