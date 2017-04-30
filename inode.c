/*
* inode.c
*/

#include "inode.h" 
#include "params.h"
#include "block.h"
#include <errno.h>





/******************************************************************************/
/******************HELPER FUNCTIONS INODE MANAGEMENT***************************/
/******************************************************************************/
/* 
	get_inode()
	read_inode()
	write_inode()
*/


/**
* 	get_inode()
* 		params: ino, empty inode_t struct 
*		return: none 
*			
*		Gets inode struct from ino and puts into second arg 
*/	
void get_inode(uint32_t ino, sfs_inode_t* rtn_inode){

	log_msg("\nget_inode()...\n");

	if (ino < SFS_N_INODES) {


		/* Is this ino in the free list?*/
		// if ((SFS_DATA->inode_cache[ino].node.next 
		// 	== SFS_DATA->inode_cache[ino].node.prev) 
		// 	&& (SFS_DATA->free_inodes 
		// 		!= &(SFS_DATA->inode_cache[ino].node))) {


		int block_offset = ino / (BLOCK_SIZE / SFS_INODE_SIZE);
		int inside_block_offset = ino % (BLOCK_SIZE / SFS_INODE_SIZE);

		char buffer[BLOCK_SIZE];
    	block_read(SFS_INODEBLOCK_INDX + block_offset, buffer);

    	int local_offset = ino%4;
    	if((((sfs_inode_t*)buffer)[local_offset]).isvalid == 0){
		    log_msg("\tError: Invalid inode number. %d\n", ino);
		    return;
    	}


	    log_msg("\tblock_offset: %d, inside_block_offset: %d", block_offset, inside_block_offset);
    	memcpy(rtn_inode, buffer + inside_block_offset*SFS_INODE_SIZE, sizeof(sfs_inode_t));
	    log_msg("\tRequested ino (%d) found\n", rtn_inode->ino);
		

		// } else {
		
		//     log_msg("\tRequested ino (%d) is not in use.\n", ino);
		
		// }
	} else {
	    log_msg("\tError: Invalid inode number. %d\n", ino);
	}
}



/**
* 	read_inode()
* 		params: inode struct, buffer, size of read, offset in read 
*		return: number of bytes read 
*			
*/
int read_inode(sfs_inode_t* inode, char* buffer, int size, int offset){

	log_msg("\nread_inode()...\n");

	int i = 0;
	int orig_offset = offset;
	char tmp_buf[BLOCK_SIZE];
	int read_byte_count = 0;
	int num_new_blocks = 0;
	int start_block_idx = (offset / BLOCK_SIZE);
	offset = offset % BLOCK_SIZE;

	for (i = start_block_idx; (read_byte_count < inode->size) && (i < SFS_DIR_PTRS);++i) {

		if (offset != 0) {
			int bytes_to_read = (BLOCK_SIZE - offset) > inode->size ? inode->size : (BLOCK_SIZE - inode->size);
			block_read(SFS_DATABLOCK_INDX + inode->blocks[i], tmp_buf);
			memcpy(buffer, tmp_buf + offset, bytes_to_read);

			read_byte_count += bytes_to_read;

			log_msg("\tOffset: %d, read %d bytes\n", offset, read_byte_count);

			offset = 0;


		} else {

			int bytes_to_read = (inode->size - read_byte_count) > BLOCK_SIZE ? BLOCK_SIZE : (inode->size - bytes_to_read);
			block_read(SFS_DATABLOCK_INDX + inode->blocks[i], tmp_buf);
			log_msg("\ttmp_buf = %s\n", tmp_buf);
			memcpy(buffer + read_byte_count, tmp_buf, bytes_to_read);

			log_msg("\tRead Buffer = %s\n", buffer);
			log_msg("\ttmp_buf1 = %s\n", tmp_buf);
			read_byte_count += bytes_to_read;

		}
	}


	return read_byte_count;
}



/**
* 	write_inode()
* 		params: inode struct, buffer, size of write, offset of write 
*		return: number of bytes written  
*			
*/
int write_inode(sfs_inode_t *inode_data, const char* buffer, int size, int offset) {

	log_msg("\nwrite_inode()...\n");

	int capacity = (SFS_DIR_PTRS - (offset / BLOCK_SIZE))*BLOCK_SIZE - (offset % BLOCK_SIZE);
	if (size > capacity) {
		log_msg("\tError: Write request too large! Limit is %d\n", capacity);
		return 0;
	}

	int i = 0;
	int orig_offset = offset;
	char tmp_buf[BLOCK_SIZE];
	int bytes_written = 0;
	int num_new_blocks = 0;
	int start_block_idx = (offset / BLOCK_SIZE);
	offset = offset % BLOCK_SIZE;

	for (i = start_block_idx; (bytes_written < size) && (i < SFS_DIR_PTRS);++i) {
		if (i >= inode_data->num_blocks) {
			// inode_data->blocks[i] = get_new_blockno();
			++num_new_blocks;
			log_msg("\tNew block being allocated to this file for the write request.\n");
		}

		if (offset != 0) {
			int bytes_to_write = (BLOCK_SIZE - offset) > size ? size : (BLOCK_SIZE - offset);
			block_read(SFS_DATABLOCK_INDX + inode_data->blocks[i], tmp_buf);
			memcpy(tmp_buf + offset, buffer, bytes_to_write);
			update_block_data(inode_data->blocks[i], tmp_buf);

			bytes_written += bytes_to_write;

			log_msg("\torig offset = %d, Offset = %d, written %d bytes\n", orig_offset, offset, bytes_written);

			offset = 0;
		} else {
			int bytes_to_write = (size - bytes_written) > BLOCK_SIZE ? BLOCK_SIZE : (size - bytes_written);
			memcpy(tmp_buf, buffer + bytes_written, bytes_to_write);
			update_block_data(inode_data->blocks[i], tmp_buf);

			log_msg("\tUpdated block %d offset = %d num bytes written = %d\n",inode_data->blocks[i], offset, bytes_to_write);

			bytes_written += bytes_to_write;
		}
	}

	inode_data->num_blocks += num_new_blocks;
	inode_data->size = orig_offset + size;

	update_inode_data(inode_data->ino, inode_data);

	return bytes_written;
}





/******************************************************************************/
/******************HELPER FUNCTIONS FOR RESOLVING PATHS************************/
/******************************************************************************/
/* 
	ino_from_path()
	ino_from_path_dir()
*/

/**
* 	ino_from_path()
* 		params: char path 
*		return: corresponding ino 
*			
*		Right now, only supports root directory. 
*/
uint32_t ino_from_path(const char *path){

	log_msg("\nino_from_path()...%s\n", path);
	if(strcmp(path, "/") == 0){
		return (SFS_DATA->root_ino);
	}else if(*path=='/'){
		return ino_from_path_dir(path+1, SFS_DATA->root_ino);
	}else {
		log_msg("\tInvalid path.\n");
	}
	return SFS_INVLD_INO;

}

uint32_t ino_from_path_dir(const char *path, uint32_t ino_parent) {
	log_msg("ino_from_path_dir()...%s\n", path);

	uint32_t ino_path = SFS_INVLD_INO;
	char buffer[SFS_DIRENTRY_SIZE * SFS_N_INODES];
	memset(buffer, 0, sizeof(buffer));

	sfs_inode_t inode;
	get_inode(ino_parent, &inode);

	int num_dentries = (inode.size / SFS_DIRENTRY_SIZE);
	if (num_dentries > 0) {
		log_msg("\tTotal direntries: %d\n", num_dentries);

		sfs_direntry_t* dentries = malloc(sizeof(sfs_direntry_t) * num_dentries);
	    read_direntries(&inode, dentries);

	    int i = 0;
	    for (i = 0; i < num_dentries; ++i) {
			log_msg("\tDirentry %d: %s, path: %s\n", i, dentries[i].name,path);
	    	if (strcmp(dentries[i].name, path) == 0) {
	    		ino_path = dentries[i].ino;
	    		log_msg("\tFound. Dentry ino: %d\n", ino_path);

	    		break;
	    	}
	    }
	    free(dentries);
	}

	return ino_path;
}

/******************************************************************************/
/******************HELPER FUNCTIONS FOR WRITING TO BLOCKS**********************/
/******************************************************************************/
/*
	update_inode_bitmap
	update_block_bitmap
	update_inode_data
	update_block_data
*/
void update_inode_bitmap(uint32_t ino, char ch) {
	int i = 0;
	char buffer[BLOCK_SIZE];
	block_read(SFS_INODE_BM_INDX + ino / BLOCK_SIZE, buffer);
	buffer[ino] = ch;
	block_write(SFS_INODE_BM_INDX + ino / BLOCK_SIZE, buffer);

	log_msg("\nupdate_inode_bitmap()... \n\tSuccessful update\n");
}

void update_block_bitmap(uint32_t bno, char ch) {
	char buffer[BLOCK_SIZE];
	block_read(SFS_DATA_BM_INDX + bno / BLOCK_SIZE, buffer);
	buffer[bno] = ch;
	block_write(SFS_DATA_BM_INDX + bno / BLOCK_SIZE, buffer);

	log_msg("\nupdate_block_bitmap()... \n\tSuccessful update\n");
}

void update_inode_data(uint32_t ino, sfs_inode_t *inode) {
	char buffer[BLOCK_SIZE];
	inode->time_mod = time(NULL);

	block_read(SFS_INODEBLOCK_INDX + ino / (BLOCK_SIZE / SFS_INODE_SIZE), buffer);
	memcpy(buffer + ((ino % (BLOCK_SIZE / SFS_INODE_SIZE)) * SFS_INODE_SIZE), inode, sizeof(sfs_inode_t));
	block_write(SFS_INODEBLOCK_INDX + ino / (BLOCK_SIZE / SFS_INODE_SIZE), buffer);

	log_msg("\nupdate_inode_data()...\n\tSuccessful update\n");
}

void update_block_data(uint32_t bno, char* buffer) {
	block_write(SFS_DATABLOCK_INDX + bno, buffer);

	log_msg("\nupdate_block_data()... \n\tSuccessful update\n");
}




/******************************************************************************/
/******************HELPER FUNCTIONS FOR FREE LIST MANAGEMENT*******************/
/******************************************************************************/
/* 
	get_new_ino()
	free_ino()
	get_new_blockno()
	free_blockno()
*/


/**
*	get_new_ino()
*		params: 
*		return: ino from free_list  
* 	REWRITE 
*/
uint32_t get_new_ino(){
	
	log_msg("\nget_new_ino()..\n");

	// // Are there any more inodes left? 
	// if(list_empty(SFS_DATA->free_inodes)){
	// 	log_msg("\tError.  No more inodes available.\n");
	// }else{

	// 	/* Grab a free ino from free_inodes */
	// 	list_node_t* my_ino = SFS_DATA->free_inodes;
	// 	/* Disconnect the node.  Relink the free_inodes pointer to new head */
	// 	list_del(SFS_DATA->free_inodes);
	// 	SFS_DATA->free_inodes = my_ino->next;

	// 	INIT_LIST_HEAD(my_ino);

	// 	/* get sfs_item struct containing my_ino */
	// 	sfs_item* ino_item = list_entry(my_ino, sfs_item, node);

	// 	if(ino_item != NULL){
	// 		log_msg("Free ino found successfully.\n");
	// 		return ino_item->id;
	// 	}else{
	// 		log_msg("\nError:  Free ino could not be found.\n");
	// 	}

	// }

	int i;
	for(i=0; i<SFS_N_INODES; i++){

		sfs_inode_t temp_inode; 
		get_inode(i, &temp_inode);

		if(temp_inode.isvalid != 1){
			return i;
		}

	}
	
	return SFS_INVLD_INO;

}


/**
*	free_ino()
*		params: ino to free 
*		return: none  
* 	Changed implementation.  DO NOT CALL THIS
*/
void free_ino(uint32_t ino){
	log_msg("\nfree_ino()...\n");


	if (ino < SFS_N_INODES){

		/* Checking if this ino is already in the free_list. */
		if((SFS_DATA->inode_cache[ino].node.next 
			== SFS_DATA->inode_cache[ino].node.prev)
			&& (SFS_DATA->free_inodes 
				!= &(SFS_DATA->inode_cache[ino].node)) ) {

			/* Add it to the tail of the free list.*/
			list_add_tail(&(SFS_DATA->inode_cache[ino].node), SFS_DATA->free_inodes);
			log_msg("\tInode successfully freed.\n");

		} else {
			log_msg("\tError: Attempted to free an already free inode.\n");
		}
	}else{
		log_msg("\tAttempted to free an invalid ino.\n");
	}
}



/**
*	new_block_no()
*		params: 
*		return: block number 
* 	Changed implementation.  DO NOT CALL THIS 
*/
uint32_t get_new_blockno(){

	log_msg("\nget_new_blockno()..\n");

	// Very similar logic to get_new_ino()
	if(list_empty(SFS_DATA->free_dblocks)){
		log_msg("\tError:  No more data blocks available.\n");
	}else{

		list_node_t* my_dblock = SFS_DATA->free_dblocks;
		list_del(SFS_DATA->free_dblocks);
		SFS_DATA->free_dblocks - my_dblock->next;

		INIT_LIST_HEAD(my_dblock);

		sfs_item* dblock_item = list_entry(my_dblock, sfs_item, node);

		if(dblock_item != NULL){
			log_msg("\tFree dblock found successfully.\n");
			return dblock_item->id;
		}else{
			log_msg("\tError: Free dblock could not be found.\n");
		}
	}

	return SFS_INVLD_DBNO;


}

/**
*	free_blockno()
*		params: blockno to free 
*		return: none  
* 	Changed implementation.  DO NOT CALL THIS 
*/
void free_blockno(uint32_t dbno){
	log_msg("\nfree_ino()...\n");

	// Very similar logic to free_ino()
	if (dbno < SFS_N_DBLOCKS){

		/* Check if this dblock is already free */
		if((SFS_DATA->dblock_cache[dbno].node.next 
			== SFS_DATA->dblock_cache[dbno].node.prev)
			&& (SFS_DATA->free_inodes 
				!= &(SFS_DATA->dblock_cache[dbno].node)) ) {

			list_add_tail(&(SFS_DATA->dblock_cache[dbno].node), SFS_DATA->free_inodes);
			log_msg("\tInode successfully freed.\n");

		} else {
			log_msg("\tError: Attempted to free an already free dblock.\n");
		}
	}else{
		log_msg("\tError: Attempted to free an invalid dblock.\n");
	}


}



/******************************************************************************/
/******************HELPER FUNCTIONS FOR DIRENTRY MANAGEMENT********************/
/******************************************************************************/
/* 
	create_direntry()
	read_direntry()
	remove_direntry()
*/

/**
* 	create_direntry()
* 		params: name, inode struct, ino of parent
*		return: none
*/
void create_direntry(const char *name, sfs_inode_t *inode, uint32_t ino_parent) {
	log_msg("\ncreate_direntry()... path=%s ino = %d ino_parent=%d\n", name, inode->ino, ino_parent);
	sfs_inode_t inode_parent;
	get_inode(ino_parent, &inode_parent);

	sfs_direntry_t dentry;
	dentry.ino = inode->ino;
	strcpy(dentry.name, name);

	char buffer[BLOCK_SIZE];

	int num_dentries = (inode_parent.size / SFS_DIRENTRY_SIZE);
	int idx = num_dentries / (BLOCK_SIZE / SFS_DIRENTRY_SIZE);
	int int_idx = num_dentries % (BLOCK_SIZE / SFS_DIRENTRY_SIZE);

	if ((int_idx == 0) && (num_dentries != 0)) {
		// inode_parent.blocks[idx] = get_new_blockno();
		update_block_bitmap(inode_parent.blocks[idx], '0');
		inode_parent.num_blocks += 1;
	}

	block_read(SFS_DATABLOCK_INDX + inode_parent.blocks[idx], buffer);
	memcpy(buffer + (int_idx * SFS_DIRENTRY_SIZE), &dentry, sizeof(sfs_direntry_t));
	block_write(SFS_DATABLOCK_INDX + inode_parent.blocks[idx], buffer);

	inode_parent.size += SFS_DIRENTRY_SIZE;
	update_inode_data(inode_parent.ino, &inode_parent);
}



/**
* 	read_direntry()
* 		params: inode struct, direntry struct
*		return: none
*/
void read_direntries(sfs_inode_t *inode_data, sfs_direntry_t* dentries) {

	log_msg("\nread_direntries()...\n");

	if (S_ISDIR(inode_data->mode)) {
		int num_blocks_read = 0;
		int num_bytes_read = 0;
		int num_entries = 0;
		int entry_offset = 0;

		while ((num_blocks_read < inode_data->num_blocks) && (num_bytes_read < inode_data->size)) {
			if (inode_data->size - num_bytes_read < BLOCK_SIZE) {
				num_entries = ((inode_data->size - num_bytes_read) / SFS_DIRENTRY_SIZE);
			} else {
				num_entries = (BLOCK_SIZE / SFS_DIRENTRY_SIZE);
			}

			log_msg("\tnum_entries=%d\n", num_entries);

			if (num_blocks_read < SFS_DIR_PTRS) {
				read_direntry_block(inode_data->blocks[num_blocks_read], dentries + entry_offset, num_entries);
			}

			++num_blocks_read;
			num_bytes_read += (num_entries * SFS_DIRENTRY_SIZE);
			entry_offset += num_entries;
		}
	} else {
	    log_msg("\tError: Given ino is not a directory inode. %d,\n", inode_data->ino);
	}
}



/**
* 	read_direntry_block()
* 		params: blockid, direntry struct, number of entries 
*		return: none
*/
void read_direntry_block(uint32_t block_id, sfs_direntry_t* dentries, int num_entries) {

	log_msg("\nread_direntry_block()...\n");

	char buffer[BLOCK_SIZE];
	block_read(SFS_DATABLOCK_INDX + block_id, buffer);

	int entries_read = 0;
	int bytes_read = 0;
	while ((bytes_read < BLOCK_SIZE) && (entries_read < num_entries)) {
		log_msg("\tEntries read = %d\n", entries_read);
		memcpy(dentries + entries_read, buffer + bytes_read, sizeof(sfs_direntry_t));
	    ++entries_read;
	    bytes_read += SFS_DIRENTRY_SIZE;
	}
}


/**
* 	remove_direntry()
* 		params: inode struct, parent ino 
*		return: none
*/
void remove_direntry(sfs_inode_t *inode, uint32_t ino_parent){
	
	log_msg("\nremove_direntry()...\n");

	sfs_inode_t inode_parent;
	get_inode(ino_parent, &inode_parent);
	if (S_ISDIR(inode_parent.mode)) {
		int num_blocks_read = 0;
		int num_bytes_read = 0;
		int num_entries = 0;
		int entry_offset = 0;

		while ((num_blocks_read < inode_parent.num_blocks)
				&& (num_bytes_read < inode_parent.size)) {

			if (inode_parent.size - num_bytes_read < BLOCK_SIZE) {
				num_entries = ((inode_parent.size - num_bytes_read) / SFS_DIRENTRY_SIZE);
			} else {
				num_entries = (BLOCK_SIZE / SFS_DIRENTRY_SIZE);
			}

			log_msg("\tRead direntries...%d entries.\n", num_entries);

			if (num_blocks_read < SFS_DIR_PTRS) {
				char buffer[BLOCK_SIZE];
				block_read(SFS_DATABLOCK_INDX + inode_parent.blocks[num_blocks_read], buffer);

				int entries_read = 0;
				int bytes_read = 0;
				while ((bytes_read < BLOCK_SIZE) && (entries_read < num_entries)) {
					log_msg("\tReading direntry from blocks.. Entries read = %d\n", entries_read);
					sfs_direntry_t dentry;
					memcpy(&dentry, buffer + bytes_read, sizeof(sfs_direntry_t));


					if (dentry.ino == inode->ino) {
						log_msg("\tFound the entry to be deleted.\n");
						// Now i am going to overwrite it with the last dentry

						int total_entries = (inode_parent.size / SFS_DIRENTRY_SIZE);
						if (total_entries > 1) {
							int idx = (total_entries - 1) / (BLOCK_SIZE / SFS_DIRENTRY_SIZE);
							int int_idx = (total_entries - 1) % (BLOCK_SIZE / SFS_DIRENTRY_SIZE);

							char buffer_last[BLOCK_SIZE];
							block_read(SFS_DATABLOCK_INDX + inode_parent.blocks[idx], buffer_last);
							sfs_direntry_t dentry_last;
							memcpy(&dentry_last, buffer_last + SFS_DIRENTRY_SIZE * int_idx, sizeof(sfs_direntry_t));

							if (int_idx == 0) {
								inode_parent.num_blocks--;
								// free_block_no(inode_parent.blocks[idx]);
								update_block_bitmap(inode_parent.blocks[idx], '1');
							}

							memcpy(buffer + bytes_read, &dentry_last, sizeof(sfs_direntry_t));
							update_block_data(inode_parent.blocks[num_blocks_read], buffer);
							inode_parent.size -= SFS_DIRENTRY_SIZE;

						} else {
							inode_parent.size -= SFS_DIRENTRY_SIZE;
						}

						update_inode_data(inode_parent.ino, &inode_parent);
						log_msg("\n Item deleted successfully");
						return;
					}
					++entries_read;
					bytes_read += SFS_DIRENTRY_SIZE;
				}
			}

			++num_blocks_read;
			num_bytes_read += (num_entries * SFS_DIRENTRY_SIZE);
			entry_offset += num_entries;
		}
	} else {
		log_msg("\n Invalid inode number %d, not a directory", inode_parent.ino);
	}
}


/******************************************************************************/
/*****************************JUST INODE STUFFS********************************/
/******************************************************************************/
/*
	create_inode()
	remove_inode()
*/

/**
* 	create_inode()
* 		params: path and mode_t
*		return: ino of the new inode
*/
uint32_t create_inode(const char* path, mode_t mode){

	log_msg("\ncreate_inode... %s\n", path);

	uint32_t temp_ino = ino_from_path(path);

	/* Check if already exists */
	if(temp_ino == SFS_INVLD_INO) {
		// Get new ino to use from free_list
		temp_ino = get_new_ino();
		// Get a block to put it in 
		// uint32_t block_no = get_new_blockno();

		if ((temp_ino != SFS_INVLD_INO)) {

			// Update inode bitmap
			update_inode_bitmap(temp_ino, '0');

			// Update data bitmap
			// update_block_bitmap(block_no, '0');

			// Create inode struct
			sfs_inode_t inode;
	       	get_inode(temp_ino, &inode);

			// memset(&inode, 0, sizeof(inode));
			inode.isvalid = 1;
			inode.time_access = inode.time_mod = inode.time_change = time(NULL);
			inode.num_blocks = 1;
			inode.ino = temp_ino;
			// inode.blocks[0] = block_no;
			inode.size = 0;
			inode.nlink = 0;
			inode.mode = mode;				

			// Write inode to disk
			update_inode_data(temp_ino, &inode);

			// Create direntry for inode
			create_direntry(path+1, &inode, ino_from_path("/"));


			return inode.ino;
		}


	}else{
		log_msg("\tError. inode already exists for given path: %s.", path);
	}


	return SFS_INVLD_INO;

}








/**
* 	remove_inode()
* 		params: path 
*		return: success
*
* 	Removes inode from and frees associated data
*/
int remove_inode(const char *path) {

	log_msg("\nremove_inode()... %s\n", path);


	uint32_t ino_path = ino_from_path(path);

	if (ino_path != SFS_INVLD_INO) {

		sfs_inode_t inode_data;
		get_inode(ino_path, &inode_data);

		int num_blocks = inode_data.num_blocks;

		while (num_blocks > 0) {

			// free_blockno(inode_data.blocks[num_blocks - 1]);
			// update_block_bitmap(inode_data.blocks[num_blocks - 1], '1');

			--num_blocks;
		}

		// free_ino(inode_data.ino);
		inode_data.isvalid = 0;
		update_inode_bitmap(inode_data.ino, '1');
		update_inode_data(inode_data.ino, &inode_data);

		log_msg("\tinode removed.");
		remove_direntry(&inode_data, SFS_DATA->root_ino);
		log_msg("\tdirentry removed.");

		return 0;
	} else {
		log_msg("\nError no such path exists!");
	}

	return -ENOENT;
}


/**
* 	populate_stat()
* 		params: inode struct, statbuffer  
*		return: none
*
* 	Helper function for get_attr.  Populates the statbuf using inode data
*/
void populate_stat(const sfs_inode_t* inode, struct stat *statbuf) {
	statbuf->st_dev = 0;
	statbuf->st_ino = inode->ino;
	statbuf->st_mode = inode->mode;
	statbuf->st_nlink = inode->nlink;
	statbuf->st_uid = getuid();
	statbuf->st_gid = getgid();
	statbuf->st_rdev = 0;
	statbuf->st_size = inode->size;
	statbuf->st_blksize = BLOCK_SIZE;
	statbuf->st_blocks = inode->num_blocks;
	statbuf->st_atime = inode->time_access;
	statbuf->st_mtime = inode->time_mod;
	statbuf->st_ctime = inode->time_change;
}
