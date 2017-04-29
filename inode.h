/*
 * inode.h
*/

#ifndef SRC_INODE_H_
#define SRC_INODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     
#include <sys/types.h>
#include <string.h>
#include <stdint.h>     /*contains int32_t*/
#include <sys/stat.h>
#include <fuse.h>
#include "block.h"

/* File System parameters */
// #define SFS_N_BLOCKS        ???     // Total number of blocks in the FS
// #define BLOCK_SIZE 512           // Defined in block.h


#define SFS_N_INODES        256     // Number of blocks for inodes in the FS
#define SFS_INODES_P_BLOCK  4       // Number of inodes per block 
#define SFS_INODE_SIZE      128     // Size of an inode struct in bytes
#define SFS_N_INODE_BLOCKS  64     // Total number of inodes in the FS

/* Specifications of inode block array */
#define SFS_DIR_PTRS        12      // Number of direct pointers
#define SFS_INDIR_PTRS      1       // Number of indirect pointers
#define SFS_DINDIR_PTRS     1       // Number of double indirect pointers
#define SFS_INDIR_INDX      SFS_DIR_PTRS
#define SFS_DINDIR_INDX     SFS_INDIR_INDX+1
#define SFS_TOTAL_PTRS      (SFS_DIR_PTRS + SFS_INDIR_PTRS + SFS_DINDIR_PTRS)   
                                    // Total number of pointers per inode

/* Number of data blocks that a pointer points to  */
#define SFS_DBLOCKS_PER_PTR  (BLOCK_SIZE / 4)
#define SFS_DIR_BLOCKS      1 * SFS_DIR_PTRS 

#define SFS_NIND_BLOCKS     (BLOCK_SIZE/4)

#define SFS_INDIR_BLOCKS    (SFS_DBLOCKS_PER_PTR * SFS_NIND_BLOCKS) // 128 Blocks = 64KB
#define SFS_DINDIR_BLOCKS   (SFS_DBLOCKS_PER_PTR * SFS_INDIR_BLOCKS) // 16384 Blocks = 8MB
#define SFS_TOTAL_BLOCKS    (SFS_DIR_BLOCKS + SFS_INDIR_BLOCKS + SFS_DINDIR_BLOCKS)
                            // Total number of data blocks supported by FS

#define SFS_N_DBLOCKS       SFS_N_INODES * SFS_DINDIR_BLOCKS       // Number of blocks for data in the FS 

/* Specifications of bitmaps.  Number of blocks required for */
#define SFS_N_DATA_BM       (SFS_N_DBLOCKS / (BLOCK_SIZE * 8))
#define SFS_N_INODE_BM      (SFS_N_INODES_TOTAL / (BLOCK_SIZE * 8))

/* BlockNum Indexes */
#define SFS_SUPERBLOCK_INDX     0       // Only requires 1 block  
#define SFS_INODE_BM_INDX       1       // ''
#define SFS_DATA_BM_INDX        2       // ''
#define SFS_INODEBLOCK_INDX     SFS_DATA_BM_INDX + SFS_N_DATA_BM 
#define SFS_DATABLOCK_INDX      SFS_INODEBLOCK_INDX + SFS_N_INODES


/* Invalid ID values for failure reporting */
#define SFS_INVLD_INO       SFS_N_INODES_TOTAL      // An invalid ino number 
#define SFS_INVLD_DBNO      SFS_N_DBLOCKS           // An invalid dblock number 

/* Limits on file and direntry names */
#define SFS_MAX_FILE_NAME_LENGTH    32      // Maximum file name length 
#define SFS_DIRENTRY_SIZE           64      // Maximum directory entry name
    
/*
    file-modes:
    https://www.gnu.org/software/libc/manual/html_node/Testing-File-Type.html
    Or maybe we just need some simple int32 or something. 
*/

typedef struct __attribute__((packed)){
    uint32_t    ino;                // inode number 
    uint32_t    mode;               // type of inode: dir/file/direct 
    uint32_t    size;               // total size of dir/file in bytes 
    uint32_t    num_blocks;         // total number of blocks 
    uint32_t    nlinks;             // number of hard links 
    /* Do we need any of these */
    uint32_t    time_access;
    uint32_t    time_mod;
    uint32_t    time_change;

    uint32_t    blocks[SFS_TOTAL_PTRS];   
                        // direction and indirection blocks (store block nums)
}sfs_inode_t;


typedef struct __attribute__((packed)) {
    uint32_t    ino;                                // inode number
    char        name[SFS_MAX_FILE_NAME_LENGTH];     // file name 

}sfs_direntry_t;



/* Functions */
uint32_t ino_from_path(const char* path);
uint32_t create_inode(const char* path, mode_t mode);


#endif