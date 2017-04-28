/*
* inode.c
*/

#include <errno.h>
#include "params.h"
#include "block.h"
#include "inode.h" 



/**
* 	ino_from_path()
* 		params: char path 
*		return: corresponding ino 
*			
*		Right now, only supports root directory. 
*/
uint32_t ino_from_path(const char *path){

	if(strcmp(path, "/") == 0){
		return (SFS_DATA->ino_root);
	}else {
		log_msg("ino_from_path:\tInvalid path.\n")
	}
	return SFS_INVLD_INO;

}


/**
*	new_block_no()
*		params: 
*		return: block number  
*
*
*/
uint32_t new_block_no(){

	if(){

	}

}



/**
*	new_ino()
*		params: 
*		return: ino from free_list  
*
*
*/
uint32_t new_ino(){
	
	// Are there any more inodes left? 
	if(list_empty(SFS_DATA0->free_inodes)){
		log_msg("\tError.  More more inodes available.\n");
	}else{

		/* Grab a free ino from free_inodes */
		list_node_t* temp = SFS_DATA->free_inodes;
		list_del(SFS_DATA->free_inodes);
		SFS_DATA->free_inodes = temp->next;

		/* Disconnect the node.  Make it point to itself */
		INIT_LIST_HEAD(temp);

		/*  */

		/*INCOMPLETE*/

	}


	return SFS_INVLD_INO;
}





/**
* 	create_inode()
* 		params: path and mode_t
*		return: ino of the new inode
*/
uint32_t create_inode(const char* path, mode_t mode){

	log_msg("\ncreate_inode: %s\n", path);

	uint_32_t temp_ino = ino_from_path(path);

	/* Check if already exists */
	if(temp_ino == SFS_INVLD_INO) {
		// Get new ino to use from free_list
		// Get a block to put it in 

		// Update inode bitmap
		// Update data bitmap
		// Create inode struct
		// Write inode to disk
		// Create direntry for inode



	}else{
		log_msg("\tError. inode already exists for given path: %s.", path)
	}


	return SFS_INVLD_INO;

}