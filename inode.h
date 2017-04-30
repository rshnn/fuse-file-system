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


#define SFS_N_INODES        4      // Number inodes in the FS
#define SFS_INODES_P_BLOCK  4       // Number of inodes per block 
#define SFS_INODE_SIZE      128     // Size of an inode struct in bytes
#define SFS_N_INODE_BLOCKS  1      // Total number of inode blocks

/* Specifications of inode block array */
#define SFS_DIR_PTRS        16      // Number of direct pointers
#define SFS_INDIR_PTRS      1       // Number of indirect pointers
#define SFS_DINDIR_PTRS     2       // Number of double indirect pointers
#define SFS_INDIR_INDX      SFS_DIR_PTRS
#define SFS_DINDIR_INDX     SFS_INDIR_INDX+1
#define SFS_TOTAL_PTRS      (SFS_DIR_PTRS + SFS_INDIR_PTRS + SFS_DINDIR_PTRS)   
                                    // Total number of pointers per inode

/* Number of data blocks that a pointer points to  */
#define SFS_DBLOCKS_PER_PTR  (BLOCK_SIZE /4)
#define SFS_DIR_BLOCKS      1 * SFS_DIR_PTRS  

#define SFS_NIND_BLOCKS     (BLOCK_SIZE/4)

#define SFS_INDIR_BLOCKS    (SFS_DBLOCKS_PER_PTR * SFS_NIND_BLOCKS) // blocks for 1 inode
#define SFS_DINDIR_BLOCKS   (SFS_DBLOCKS_PER_PTR * SFS_INDIR_BLOCKS) // blocks for 1 inode

#define SFS_TOTAL_BLOCKS    (SFS_DIR_BLOCKS + SFS_INDIR_PTRS*SFS_INDIR_BLOCKS + SFS_DINDIR_PTRS*SFS_DINDIR_BLOCKS)
                            // Total number of data blocks supported by 1 inode
#define SFS_TOTAL_BLOCKS_FS SFS_TOTAL_BLOCKS * SFS_N_INODES     // Total number of inodes supported by FS


#define SFS_N_DBLOCKS       SFS_N_INODES * SFS_DINDIR_BLOCKS       // Number of blocks for data in the FS 


/* Specifications of bitmaps.  Number of blocks required for */
#define SFS_N_DATA_BM       (SFS_N_DBLOCKS / (BLOCK_SIZE * 8))
#define SFS_N_INODE_BM      1

/* BlockNum Indexes */
#define SFS_SUPERBLOCK_INDX     0       // Only requires 1 block  
// #define SFS_INODE_BM_INDX       1       // ''
// #define SFS_DATA_BM_INDX        2       // ''
#define SFS_INODEBLOCK_INDX     1    // 
#define SFS_DATABLOCK_INDX      SFS_INODEBLOCK_INDX + SFS_N_INODE_BLOCKS


/* Invalid ID values for failure reporting */
#define SFS_INVLD_INO       SFS_N_INODES      // An invalid ino number 
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
    uint32_t    isvalid;            // IS THIS VALID?
    uint32_t    ino;                // inode number 
    uint32_t    mode;               // type of inode: dir/file/direct 
    uint32_t    size;               // total size of dir/file in bytes 
    uint32_t    num_blocks;         // total number of blocks 
    uint32_t    nlink;              // number of hard links 
    uint32_t    current_unused_indx;     // index of blocks array thats is currently unsed
    uint32_t    current_unused_bno; // blockno of next unsed block allocated to this inode    
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

void get_inode(uint32_t ino, sfs_inode_t* rtn_inode);
int read_inode(sfs_inode_t* inode, char* buffer, int size, int offset);
int write_inode(sfs_inode_t *inode_data, const char* buffer, int size, int offset);

uint32_t ino_from_path(const char *path);
uint32_t ino_from_path_dir(const char *path, uint32_t ino_parent);

void update_inode_bitmap(uint32_t ino, char ch);
void update_block_bitmap(uint32_t bno, char ch);
void update_inode_data(uint32_t ino, sfs_inode_t *inode);
void update_block_data(uint32_t bno, char* buffer);

uint32_t get_new_ino();
void free_ino(uint32_t ino);
uint32_t get_new_blockno();
void free_blockno(uint32_t dbno);


void create_direntry(const char *name, sfs_inode_t *inode, uint32_t ino_parent);
void read_direntries(sfs_inode_t *inode_data, sfs_direntry_t* dentries);
void read_direntry_block(uint32_t block_id, sfs_direntry_t* dentries, int num_entries);
void remove_direntry(sfs_inode_t *inode, uint32_t ino_parent);


uint32_t create_inode(const char* path, mode_t mode);
int remove_inode(const char *path);
void populate_stat(const sfs_inode_t* inode, struct stat *statbuf);

#endif