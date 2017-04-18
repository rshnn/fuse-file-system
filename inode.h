/*
 * inode.h
*/

#ifndef SRC_INODE_H_
#define SRC_INODE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>     
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>     /*contains int32_t*/
#include <fuse.h>


/* File System parameters */
#define SFS_N_BLOCKS        256     // Total number of inodes

#define SFS_N_INODES        
#define SFS_N_DBLOCKS               


/*Specifications of inode block array */
#define SFS_DIR_BLOCK       12      // Number of direct blocks
#define SFS_INDIR_BLOCKS    1       // Number of indirect blocks
#define SFS_DINDIR_BLOCKS   1       // Number of double indirect blocks
#define SFS_INODE_N_BLOCKS  SFS_DIR_BLOCK + SFS_INDIR_BLOCKS + SFS_DINDIR_BLOCKS   
                                    // Total number of direct/indirect blocks


#define SFS_MAX_FILE_NAME_LENGTH    32      // Maximum file length 

#define SFS_INVLD_INO         401     // An invalid ino number 
#define SFS_INVLD_DBLOCK_NO   




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
    // char        filename[SYS_MAX_FILE_NAME_SIZE]

    /* Do we need any of these */
    uint32_t    time_access;
    uint32_t    time_mod;
    uint32_t    time_change;

    uint32_t    blocks[SFS_N_BLOCKS_INODE];   // direction and indirection blocks (store block nums)

}sfs_inode_t;


typedef struct __attribute__((packed)) {
    uint32_t    ino;
    char        filenames[SFS_MAX_FILE_NAME_LENGTH]

}sfs_dentry_t;



/* Functions */
uint_32_t ino_from_path(const char* path);
uint_32_t create_inode(const char* path, mode_t mode);


#endif