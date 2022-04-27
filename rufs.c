/*
 *  Copyright (C) 2022 CS416/518 Rutgers CS
 *	RU File System
 *	File:	rufs.c
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "rufs.h"

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here
struct superblock superblock;

bitmap_t inode_bitmap;
bitmap_t data_block_bitmap;

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {

    // Step 1: Read inode bitmap from disk

    // Step 2: Traverse inode bitmap to find an available slot

    // Step 3: Update inode bitmap and write to disk 

	if(inode_bitmap == NULL){
		inode_bitmap = malloc((MAX_INUM / 8));
	}
    char block[BLOCK_SIZE];
    bio_read(superblock.i_bitmap_blk, &block);
	memcpy(inode_bitmap, &block, (MAX_INUM / 8));
    //call bio read to convert
    // 0 is free 
    int index = superblock.max_inum + 1;
    for(int i = 0; i<superblock.max_inum;i++){
        if(get_bitmap(inode_bitmap, i) == 0){
            index = i;
			break;
        }
    }
	if(index == superblock.max_inum + 1){
		return -1;
	}
	set_bitmap(inode_bitmap, index);
	memcpy(&block, inode_bitmap, (MAX_INUM / 8));
	bio_write(superblock.i_bitmap_blk, &block);
	return index;
}
/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

    // Step 1: Read data block bitmap from disk

    // Step 2: Traverse data block bitmap to find an available slot

    // Step 3: Update data block bitmap and write to disk 

    char block[BLOCK_SIZE];
    bio_read(superblock.d_bitmap_blk, &block);
    memcpy(data_block_bitmap, &block, (MAX_INUM / 8));
    //call bio read to convert
    // 0 is free 
    int index = superblock.max_dnum + 1;
    for(int i = 0; i<superblock.max_dnum;i++){
        if(get_bitmap(data_block_bitmap, i) == 0){
            index = i;
            break;
        }
    }
    if(index == superblock.max_inum + 1){
        return -1;
    }
    set_bitmap(data_block_bitmap, index);
    memcpy(&block, data_block_bitmap, (MAX_INUM / 8));
    bio_write(superblock.d_bitmap_blk, &block);
    return index;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {
  	// Step 1: Get the inode's on-disk block number
	int inode_block_index = ino / (int)IBLOCK_SIZE;
	
	char inode_block[BLOCK_SIZE];
	bio_read(superblock.i_start_blk + inode_block_index, &inode_block);

	// Step 2: Get offset of the inode in the inode on-disk block
	int offset = ino % (int)IBLOCK_SIZE;

	// Step 3: Read the block from disk and then copy into inode structure
	memcpy(inode, &inode_block[offset * sizeof(struct inode)], sizeof(struct inode));
	
	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	int inode_block_index = ino / (int)IBLOCK_SIZE;
	
	char inode_block[BLOCK_SIZE];
	bio_read(superblock.i_start_blk + inode_block_index, &inode_block);

	// Step 2: Get the offset in the block where this inode resides on disk
	int offset = ino % (int)IBLOCK_SIZE;

	// Step 3: Write inode to disk 
	memcpy(&inode_block[offset * sizeof(struct inode)], inode, sizeof(struct inode));
	bio_write(superblock.i_start_blk + inode_block_index, &inode_block);

	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

  // Step 2: Get data block of current directory from inode

  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure

	return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	
	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

    readi(f_ino, dir_inode);
	if(dirent(dir_inode.direct_ptr))
	struct dirent* entries = dirent(dir_inode.direct_ptr);



	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	int data_block_index = dir_inode.direct_ptr[0] / BLOCK_SIZE;

	char dirent_block[BLOCK_SIZE];
	bio_read(data_block_index, &dirent_block);

	struct dirent *entry_ptr = &dirent_block;
	int max_num_entries = BLOCK_SIZE / sizeof(struct dirent);

	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk
	for (int i = 0; i < max_num_entries; i++) {
		if (entry_ptr->valid && strcmp(entry_ptr->name, fname) == 0) {
			entry_ptr->valid = 0;
			bio_write(data_block_index, dirent_block);
			return 1;
		}
		entry_ptr++;
	}

	return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way
	char *file = strtok(path, "/");

	while( file != NULL ) {
		struct dirent dirent;
		if (dir_find(ino, file, strlen(file), &dirent)) {
			// Do stuff
		}
		file = strtok(NULL, "/");
	}

	return 0;
}

/* 
 * Make file system
 */
int rufs_mkfs() {
	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);

	// write superblock information
	superblock.magic_num = MAGIC_NUM;
	superblock.max_inum = MAX_INUM;
	superblock.max_dnum = MAX_DNUM;
	superblock.i_bitmap_blk = 1;
	superblock.d_bitmap_blk = 2;
	superblock.i_start_blk = 3;
	superblock.d_start_blk = 3 + (int) MAX_INUM * sizeof(struct inode) / ((int) BLOCK_SIZE);

	char superblock_buf[BLOCK_SIZE];
	memcpy(superblock_buf, &superblock, sizeof(superblock));

	bio_write(0, superblock_buf);

	// initialize inode bitmap
	inode_bitmap = malloc(MAX_INUM / 8);
	for (int i = 0; i < MAX_INUM / 8; i++) {
		unset_bitmap(inode_bitmap, i);
	}

	// initialize data block bitmap
	data_block_bitmap = malloc(MAX_DNUM / 8);
	for (int i = 0; i < MAX_DNUM / 8; i++) {
		unset_bitmap(data_block_bitmap, i);
	}

	// update bitmap information for root directory
	set_bitmap(inode_bitmap, 0);
	set_bitmap(data_block_bitmap, 0);

	char inode_bitmap_block[BLOCK_SIZE];
	memcpy(inode_bitmap_block, inode_bitmap, MAX_INUM / 8);
	bio_write(1, inode_bitmap);

	char data_bitmap_block[BLOCK_SIZE];
	memcpy(data_bitmap_block, data_block_bitmap, MAX_DNUM / 8);
	bio_write(2, data_block_bitmap);

	// update inode for root directory
	struct inode root_directory;

	root_directory.ino = 0;
	root_directory.valid = 1;
	root_directory.type = DIRECTORY;
	root_directory.size = 0;
	root_directory.direct_ptr[0] = superblock.d_start_blk * BLOCK_SIZE;

	char root_directory_inode_block[BLOCK_SIZE];
	memcpy(root_directory_inode_block, &root_directory, sizeof(root_directory));
	bio_write(3, root_directory_inode_block);

	return 0;
}


/* 
 * FUSE file operations
 */
static void *rufs_init(struct fuse_conn_info *conn) {
	if (dev_open(diskfile_path) == -1) {
		// Step 1a: If disk file is not found, call mkfs
		rufs_mkfs();
	} else {
		// Step 1b: If disk file is found, just initialize in-memory data structures
		// and read superblock from disk

		char block[BLOCK_SIZE];
		bio_read(0, block);
		
		memcpy(&superblock, block, sizeof(superblock));

		inode_bitmap = malloc(superblock.max_inum / 8);
		data_block_bitmap = malloc(superblock.max_dnum / 8);
	}

  

	return NULL;
}

static void rufs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	free(inode_bitmap);
	free(data_block_bitmap);

	// Step 2: Close diskfile
	dev_close(diskfile_path);

}

static int rufs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode

		stbuf->st_mode   = S_IFDIR | 0755;
		stbuf->st_nlink  = 2;
		time(&stbuf->st_mtime);

	return 0;
}

static int rufs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

    return 0;
}

static int rufs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	return 0;
}


static int rufs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory

	// Step 5: Update inode for target directory

	// Step 6: Call writei() to write inode to disk
	

	return 0;
}

static int rufs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	return 0;
}

static int rufs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int rufs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int rufs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int rufs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int rufs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of target file

	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

	return 0;
}

static int rufs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int rufs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations rufs_ope = {
	.init		= rufs_init,
	.destroy	= rufs_destroy,

	.getattr	= rufs_getattr,
	.readdir	= rufs_readdir,
	.opendir	= rufs_opendir,
	.releasedir	= rufs_releasedir,
	.mkdir		= rufs_mkdir,
	.rmdir		= rufs_rmdir,

	.create		= rufs_create,
	.open		= rufs_open,
	.read 		= rufs_read,
	.write		= rufs_write,
	.unlink		= rufs_unlink,

	.truncate   = rufs_truncate,
	.flush      = rufs_flush,
	.utimens    = rufs_utimens,
	.release	= rufs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &rufs_ope, NULL);

	return fuse_stat;
}

