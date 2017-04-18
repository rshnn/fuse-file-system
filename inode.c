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
		log_msg("\nino_from_path:\n\tInvalid path.\n")
	}
	return SFS_INVLD_INO;

}


uint32_t





/**
* 	create_inode()
* 		params: path and mode_t
*		return: ino of the new inode
*/
uint32_t create_inode(const char* path, mode_t mode){

	uint_32_t temp_ino = ino_from_path(path);

	/*Check if valid*/
	if(temp_ino != SFS_INVLD_INO && {

	}

}