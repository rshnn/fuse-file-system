/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif


#include "log.h"
#include "inode.h"
#include "inode.c"

#define SFS_SUPERSECRET 518

typedef struct __attribute__((packed)){
    uint32_t secret_number;     // Validates if currently mounted FS is init'd

    uint32_t N_data_blocks;     // Number of data blocks allocated
    uint32_t N_free_blocks;     // Number of free blocks available
    
    uint32_t N_inodes;          // Number of inodes total
    uint32_t N_inodes_used;     // Number of inodes allocated
    uint32_t N_inodes_avail;    // Number of inodes available

    uint32_t root_ino;          // Root dir's ino
    sfs_inode_t root_inode;    // Root's inode struct

}SuperDuperBlock;


///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "in bb-init\n");
    log_msg("\nsfs_init()...\n");
    log_conn(conn);
    log_fuse_context(fuse_get_context());
    
    disk_open(SFS_DATA->diskfile);
    log_msg("\tSuccessfully opened file\n");

    struct stat* root_stat = (struct stat*)malloc(sizeof(struct stat));
    lstat(SFS_DATA->diskfile, root_stat);

    /* Initialize diskfile structure*/
    if(root_stat->st_size == 0){

        /*DO INIT STUFF*/


        // Init bitmaps (inode and data)
        int i = 0;
        char bitmap_inodes[BLOCK_SIZE];
        memset(bitmap_inodes, '1', sizeof(bitmap_inodes));
        for (i = 0; i < SFS_N_INODE_BM; ++i) {
            block_write((SFS_INODE_BM_INDX + i), bitmap_inodes);
        }
        
        char bitmap_data[BLOCK_SIZE];
        memset(bitmap_data, '1', sizeof(bitmap_data));
        for (i = 0; i < SFS_N_DATA_BM; ++i) {
            block_write((SFS_DATA_BM_INDX + i), bitmap_data);
        }

        log_msg("\tFinished bitmap inits\n");


        // Init inode_block
        char inode_buffer[BLOCK_SIZE];
        memset(inode_buffer, '0', sizeof(inode_buffer));

        for(i=0; i < (SFS_N_INODES/4); ++i){
            block_write((SFS_INODEBLOCK_INDX+i), inode_buffer);
        }    
        log_msg("\tFinished inode block init\n");




        // Init data_block 
        char dblock_buffer[BLOCK_SIZE];
        memset(dblock_buffer, 0, sizeof(dblock_buffer));
        /*Following for loop is to mass-allocate a fized number of dblocks*/
        // for(i=0; i < SFS_N_DBLOCKS; ++i){
        //     block_write((SFS_DATABLOCK_INDX+i), dblock_buffer);
        // }    


        /*Following giant for loop allocates dblocks and inits inode.blocks*/
        int j, k, l, m, n, t;
        int curr = SFS_DATABLOCK_INDX;

        for(i=0; i< SFS_N_INODES; ++i){
            /*Populating newInode.blocks with blockno's of allocated blocks. */
            sfs_inode_t newInode;
            memset(&newInode, 0, sizeof(sfs_inode_t));
            
            int idx = 0;
            //uint32_t blocks_temp[SFS_TOTAL_PTRS]; //array of pointers in inode
            newInode.blocks[idx];

            for(j=0; i< SFS_DIR_PTRS; ++j){
                /*For each direct pointer*/
                block_write(curr, dblock_buffer);

                log_msg("\tI am writing a dblock. %d.\n", curr);
                
                //blocks_temp[idx] = curr;
                newInode.blocks[idx] = curr;
                idx++;
                curr++;
            }


            // for(k=0; k < SFS_INDIR_PTRS; ++k){
            //     /*For each indirect pointer */
            //     blocks_temp[idx] = curr;
            //     idx++;
                    
            //     int indir_buffer[BLOCK_SIZE/4]; //block of block integers

            //     for(l=0; l<(BLOCK_SIZE/4); ++l){
            //         /*For each block pointed to*/

            //         indir_buffer[l] = curr;
            //         block_write(curr, dblock_buffer);
            //         curr++;
            //     }

            //     block_write(blocks_temp[idx-1], indir_buffer);


            // }

            // for(m=0; m< SFS_DINDIR_PTRS; ++m){
            //     /* For each double indir pointer*/

            //     int dindir_buffer[BLOCK_SIZE/4];

            //     blocks_temp[idx] = curr;//block to store 128 single indirect pointers
            //     idx++;



            //     for(k=0; k < SFS_INDIR_PTRS; ++k){
            //         /*For each indirect pointer */
            //       // blocks_temp[idx] = curr;
            //       // idx++;
            //         dindir_buffer[k] = curr;   
            //         int indir_buffer[BLOCK_SIZE/4];

            //         for(l=0; l<(BLOCK_SIZE/4); ++l){
            //             /*For each block pointed to*/

            //             indir_buffer[l] = curr;
            //             block_write(curr, dblock_buffer);
            //             curr++;
            //         }

            //         block_write(dindir_buffer[k], indir_buffer);

            //     }

            // }

            /*Write new inode (+blocks) back */

            update_inode_data(SFS_INODEBLOCK_INDX+ i, &newInode);
            

        }

        log_msg("\tFinished allocating dblocks\n");





        // Init root inode ("/")

        if (block_read(SFS_INODE_BM_INDX, bitmap_inodes) > 0) {
            bitmap_inodes[0] = '0';
            block_write(SFS_INODE_BM_INDX, bitmap_inodes);
        }

        if (block_read(SFS_DATA_BM_INDX, bitmap_data) > 0) {
            bitmap_data[0] = '0';
            block_write(SFS_DATA_BM_INDX, bitmap_data);
        }



        /* get inode 0 */
        sfs_inode_t inode;
        get_inode(0, &inode);
        /* write stuff to it */


        memset(&inode, 0, sizeof(inode));
        inode.isvalid = 1;
        inode.time_access = inode.time_change = inode.time_mod = time(NULL);
        inode.num_blocks = 1;
        inode.ino = 0;
        inode.size = 0;
        inode.nlink = 0;
        inode.mode = S_IFDIR;
        // inode.blocks[0] = SFS_BLOCK_DATA;

        /* write it black */
        update_inode_data(0, &inode);
        SFS_DATA->root_ino = 0;
        log_msg("\tFinished writing root inode.\n");

        /*</root_inode>*/



        // Init superblock
        SuperDuperBlock SuperBlock = {
            .secret_number = SFS_SUPERSECRET,
            .N_data_blocks = 0, 
            .N_free_blocks = SFS_N_DBLOCKS, 
            .N_inodes = SFS_N_INODES, 
            .N_inodes_used = 0, 
            .N_inodes_avail = SFS_N_INODES, 
            .root_ino = 0,
            .root_inode = inode
        };

        block_write_padded(SFS_SUPERBLOCK_INDX, &SuperBlock, sizeof(SuperDuperBlock));
        log_msg("\tFinished setting SuperBlock.\n");


    }else{
        // Check the secret number 
        char secret_buffer[BLOCK_SIZE];
        block_read(SFS_SUPERBLOCK_INDX, secret_buffer);
        if (((SuperDuperBlock*)(secret_buffer))->secret_number != SFS_SUPERSECRET){
            /* Failed magic number test */
            log_msg("\n#NotMyFS\n");
            exit(-1);
        }
        log_msg("\tFile system already initialized.  Woopwoop\n");

    }


    log_conn(conn);
    log_fuse_context(fuse_get_context());

    
    return SFS_DATA;
    
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{
    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
    disk_close();

    /* Free fuse_context stuff */

}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);
    
    uint32_t ino = ino_from_path(path);
    if(ino != SFS_INVLD_INO){
        log_msg("\tPath found.\n");
        sfs_inode_t inode;
        get_inode(ino, &inode);

        /*Found the inode, now fill out the statbuf*/
        populate_stat(&inode, statbuf);
    }else{
        log_msg("\tError:  Path was not found.\n");
    }

    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);

    /* Redirect this to the inode.c implementation of create_inode() */    
    uint32_t ino = create_inode(path, mode);
    log_msg("\tFile was successfully creted with ino: %d.\n", ino);

    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
    int retstat = 0;
    log_msg("sfs_unlink(path=\"%s\")\n", path);
    retstat = remove_inode(path);
    
    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);

    retstat = -ENOENT;

    uint32_t ino = ino_from_path(path);

    if(ino != SFS_INVLD_INO){
        sfs_inode_t inode;
        get_inode(ino, &inode);

        if(S_ISREG(inode.mode)){
            retstat = 0;
        }
    }else{
        log_msg("\tProvided path does not resolve to a valid file.\n");
    }

    
    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    

    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);

   
    uint32_t ino = ino_from_path(path);

    if (ino != SFS_INVLD_INO) {
        log_msg("\tPath resolved to an ino.\n");
        sfs_inode_t inode;
        get_inode(ino, &inode);

        log_msg("\tInode was successfully retrieved.\n");

        retstat = read_inode(&inode, buf, size, offset);
        log_msg("\tData read = %s\n", buf);
    } else {
        log_msg("\tError: Path does not resolve to a valid ino.\n");
        retstat = -ENOENT;
    }



    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
        path, buf, size, offset, fi);


    uint32_t ino = ino_from_path(path);

    if (ino != SFS_INVLD_INO) {

        log_msg("\tPath resolved to an ino.\n");
        sfs_inode_t inode;
        get_inode(ino, &inode);
        log_msg("\tInode was successfully retrieved.\n");

        retstat = write_inode(&inode, buf, size, offset);

    } else {
        log_msg("\tError: Path does not resolve to a valid ino.\n");
        retstat = -ENOENT;
    }
    
    return retstat;

}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
   
   /*Same as creating regular file.  Create inode. */
    uint32_t ino = create_inode(path, mode);
    log_msg("\tDirectory creation success. ino = %d\n", ino);
    
    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n",
	    path);
    
    
    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
        

    uint32_t ino = ino_from_path(path);
    
    if (ino != SFS_INVLD_INO) {

        log_msg("\tPath resolved to an ino.\n");
        
        sfs_inode_t inode;
        get_inode(ino, &inode);
        log_msg("\tInode was successfully retrieved.\n");

        if (S_ISDIR(inode.mode)) {
            retstat = 0;
        }

    } else {
        log_msg("\nNot a valid file");
    }
    
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    
    log_msg("\nsfs_readdir(path=\"%s\")...\n", path);
    int retstat = 0;
    

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    uint32_t ino = ino_from_path(path);
    if (ino != SFS_INVLD_INO) {

        log_msg("\tPath resolved to an ino.\n");
        
        sfs_inode_t inode;
        get_inode(ino, &inode);
        log_msg("\tSuccessfully retrieved the inode.\n");

        int num_dentries = (inode.size / SFS_DIRENTRY_SIZE);
        sfs_direntry_t* dentries = malloc(sizeof(sfs_direntry_t) * num_dentries);
        read_direntries(&inode, dentries);

        int i = 0;
        for (i = 0; i < num_dentries; ++i) {
            filler(buf, dentries[i].name, NULL, 0);
        }

        free(dentries);
    } else {
        log_msg("\tError: Path did not resolve to an ino.\n");
    }


    
    return retstat;
}









/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    
    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage()
{
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct sfs_state *sfs_data;
    
    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the diskfile and save it in internal data
    // sfs_data->diskfile = argv[argc-2];
    sfs_data->diskfile = realpath(argv[argc-2], NULL);
    printf("%s\n", sfs_data->diskfile);

    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    sfs_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}



/* SCP'ED THIS IN */


