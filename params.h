/*
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  There are a couple of symbols that need to be #defined before
  #including all the headers.
*/

#ifndef _PARAMS_H_
#define _PARAMS_H_

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26

// need this to get pwrite().  I have to use setvbuf() instead of
// setlinebuf() later in consequence.
#define _XOPEN_SOURCE 500

// maintain bbfs state in here
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include "list.h"

typedef struct{
	int id;  			// id = ino for inodes or blocknum for dblocks
	list_node_t node;
}sfs_item;



struct sfs_state {
    FILE *logfile;
    char *diskfile;

    sfs_item* inode_cache;		// Array of sfs_items for all inodes
    sfs_item* dblock_cache;		// Array of sfs_items for all dblocks

  	list_node_t* free_inodes; 	// List of free inos (connected via sfs_item->node->next)
  	list_node_t* free_dblocks;  // Same for dblocks 

    uint32_t root_ino; 			// ino of root dir.  "/"
};

#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)

#endif
