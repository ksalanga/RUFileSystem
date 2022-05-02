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
bitmap_t data_bitmap;

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
    memcpy(data_bitmap, &block, (MAX_INUM / 8));
    //call bio read to convert
    // 0 is free 
    int index = superblock.max_dnum + 1;
    for(int i = 0; i<superblock.max_dnum;i++){
        if(get_bitmap(data_bitmap, i) == 0){
            index = i;
            break;
        }
    }
    if(index == superblock.max_inum + 1){
        return -1;
    }
    set_bitmap(data_bitmap, index);
    memcpy(&block, data_bitmap, (MAX_INUM / 8));
    bio_write(superblock.d_bitmap_blk, &block);
    return index;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {
  	// Step 1: Get the inode's on-disk block number
	int inode_block_index = ino / ((int)IBLOCK_SIZE);
	
	char inode_block[BLOCK_SIZE];
	bio_read(superblock.i_start_blk + inode_block_index, &inode_block);

	// Step 2: Get offset of the inode in the inode on-disk block
	int offset = ino % ((int)IBLOCK_SIZE);


	// Step 3: Read the block from disk and then copy into inode structure
	memcpy(inode, &inode_block[offset * sizeof(struct inode)], sizeof(struct inode));

	return 0;
}

int writei(uint16_t ino, struct inode *inode) {
	// Step 1: Get the block number where this inode resides on disk
	int inode_block_index = ino / ((int)IBLOCK_SIZE);
	
	char inode_block[BLOCK_SIZE];
	bio_read(superblock.i_start_blk + inode_block_index, &inode_block);


	// Step 2: Get the offset in the block where this inode resides on disk
	int offset = ino % ((int)IBLOCK_SIZE);

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
	struct inode inode;
  	readi(ino, &inode);

  // Step 2: Get data block of current directory from inode
	int data_block_index = inode.direct_ptr[0] / BLOCK_SIZE;
	char dirent_block[BLOCK_SIZE];
	bio_read(data_block_index, &dirent_block);
	struct dirent *entry_ptr = (struct dirent *)&dirent_block;
	int max_num_entries = BLOCK_SIZE / sizeof(struct dirent);


  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure

  	for (int i = 0; i < max_num_entries; i++) {
		if (entry_ptr->valid && strcmp(entry_ptr->name, fname) == 0) {
			memcpy(dirent, entry_ptr, sizeof(struct dirent));
			return 1;
		}
		entry_ptr++;
	}

	return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	
	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry
	int data_block_index = dir_inode.direct_ptr[0] / BLOCK_SIZE;
	char dirent_block[BLOCK_SIZE];
	bio_read(data_block_index, &dirent_block);
	struct dirent *entry_ptr = (struct dirent *)&dirent_block;

	int numEntries = BLOCK_SIZE / sizeof(struct dirent);

	for(int i = 0; i < numEntries; i++){
		if (entry_ptr->valid && strcmp(entry_ptr->name, fname) == 0) {
			return 1;
		}else if(!entry_ptr->valid){
			entry_ptr->valid = 1;
			entry_ptr->len = name_len;
			entry_ptr->ino = f_ino;
			strcpy(entry_ptr->name, fname);
			bio_write(data_block_index, &dirent_block);
			return 1;
		}
		entry_ptr++;
	}



	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	int data_block_index = dir_inode.direct_ptr[0] / BLOCK_SIZE;

	char dirent_block[BLOCK_SIZE];
	bio_read(data_block_index, &dirent_block);

	struct dirent *entry_ptr = (struct dirent *)&dirent_block;
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
	if (strcmp(path, "/") == 0) {
		readi(0, inode);
		return 1;
	}

	char *file = strtok((char *)path, "/");

	struct dirent dirent;
	dirent.valid = 0;
	while (file != NULL) {
		if (dir_find(ino, file, strlen(file), &dirent)) {
			ino = dirent.ino;
		} else {
			return 0;
		}
		file = strtok(NULL, "/");
	}

	if (dirent.valid) {
		readi(dirent.ino, inode);
		return 1;
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
	data_bitmap = malloc(MAX_DNUM / 8);
	for (int i = 0; i < MAX_DNUM / 8; i++) {
		unset_bitmap(data_bitmap, i);
	}

	// update bitmap information for root directory
	set_bitmap(inode_bitmap, 0);
	set_bitmap(data_bitmap, 0);

	char inode_bitmap_block[BLOCK_SIZE];
	memcpy(&inode_bitmap_block, inode_bitmap, MAX_INUM / 8);
	bio_write(1, inode_bitmap);

	char data_bitmap_block[BLOCK_SIZE];
	memcpy(&data_bitmap_block, data_bitmap, MAX_DNUM / 8);
	bio_write(2, data_bitmap);

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
		data_bitmap = malloc(superblock.max_dnum / 8);
	}

  

	return NULL;
}

static void rufs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	free(inode_bitmap);
	free(data_bitmap);

	// Step 2: Close diskfile
	dev_close(diskfile_path);

}

static int rufs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode

	struct inode inode;
	int check = get_node_by_path(path,0, &inode);
	if(check == 0){
		return -ENOENT; // not found
	}
		if (inode.type == REG_FILE) {
			stbuf->st_mode = S_IFREG | 0644;
		} else {
			stbuf->st_mode = S_IFDIR | 0755;
		}
		stbuf->st_nlink  = inode.link;
		stbuf->st_size = inode.size;
		time(&stbuf->st_mtime);
		stbuf->st_uid = getuid();
		stbuf->st_gid = getgid();
		

	return 0;
}

static int rufs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1
	struct inode inode;
	
	int check = get_node_by_path(path,0, &inode);

	if(check == 0){
		return -ENOENT; // not found
	}

    return 0;
}

static int rufs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	struct inode inode;
	int check = get_node_by_path(path,0, &inode);
	if(check == 0){
		return -ENOENT; // not found
	}

	int data_block_index = inode.direct_ptr[0] / BLOCK_SIZE;
	char dirent_block[BLOCK_SIZE];
	bio_read(data_block_index, &dirent_block);
	struct dirent *entry_ptr = (struct dirent *)&dirent_block;

	int numEntries = BLOCK_SIZE / sizeof(struct dirent);
	for(int i = 0; i < numEntries; i++){
		if(entry_ptr->valid){
			filler(buffer, entry_ptr->name, NULL, 0);
		}
		entry_ptr++;
	}


	return 0;
}


static int rufs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char *lastSlash = strrchr((char *)path, '/');
    const char *base = lastSlash ? lastSlash + 1 : path;

    char dir[lastSlash + 1 - path];
	memcpy(&dir, path, lastSlash + 1 - path);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode dir_inode;
	if (get_node_by_path(dir, 0, &dir_inode)) {
		// Step 3: Call get_avail_ino() to get an available dir_inode number
		int avail_ino = get_avail_ino();
		int avail_block = get_avail_blkno();

		if (avail_ino != -1 && avail_block != -1) {
			// Step 4: Call dir_add() to add directory entry of target directory to parent directory
			dir_add(dir_inode, avail_ino, base, sizeof(base));

			// Step 5: Update inode for target directory
			struct inode target_dir_inode;
			target_dir_inode.ino = avail_ino;
			target_dir_inode.valid = 1;
			target_dir_inode.type = DIRECTORY;
			target_dir_inode.size = 0;
			target_dir_inode.direct_ptr[0] = (superblock.d_start_blk + avail_block) * BLOCK_SIZE;

			// Step 6: Call writei() to write inode to disk
			writei(avail_ino, &target_dir_inode);

			return 0;
		}
	}
	

	return -1;
}

static int rufs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char *lastSlash = strrchr(path, '/');
    const char *base = lastSlash ? lastSlash + 1 : path;

    char dir[lastSlash + 1 - path];
	memcpy(&dir, path, lastSlash + 1 - path);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode target_dir_inode;
	if (get_node_by_path(path, 0, &target_dir_inode)) {
		// Step 3: Clear data block bitmap of target directory
		int data_bitmap_index = (target_dir_inode.direct_ptr[0] - superblock.d_start_blk * BLOCK_SIZE) / BLOCK_SIZE;
		unset_bitmap(data_bitmap, data_bitmap_index);
		char data_bitmap_block[BLOCK_SIZE];
		memcpy(&data_bitmap_block, data_bitmap, superblock.max_dnum / 8);
		bio_write(superblock.d_bitmap_blk, &data_bitmap_block);

		// Step 4: Clear inode bitmap and its data block
		unset_bitmap(inode_bitmap, target_dir_inode.ino);
		char inode_bitmap_block[BLOCK_SIZE];
		memcpy(&inode_bitmap_block, inode_bitmap, superblock.max_inum / 8);
		bio_write(superblock.i_bitmap_blk, &inode_bitmap_block);

		// Step 5: Call get_node_by_path() to get inode of parent directory
		struct inode dir_inode;
		get_node_by_path((const char *)&dir, 0, &dir_inode);

		// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory
		dir_remove(dir_inode, base, strlen(base));
		return 1;
	}

	return 0;
}

static int rufs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_create(const char *path, mode_t mode, struct fuse_file_info *fi) { 

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char *dirc, *basec, *bname, *dname;

	dirc = strdup(path);
	basec = strdup(path);
	dname = dirname(dirc);
	bname = basename(basec);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode target_dir_inode;
	if (get_node_by_path(dname, 0, &target_dir_inode)) {
		// Step 3: Call get_avail_ino() to get an available inode number
		int avail_ino = get_avail_ino();
		int avail_block = get_avail_blkno();

		if (avail_ino != -1 && avail_block != -1) {
			// Step 4: Call dir_add() to add directory entry of target directory to parent directory
			dir_add(target_dir_inode, avail_ino, bname, strlen(bname + '\0'));
			// Step 5: Update inode for target file
			struct inode target_file_inode;
			target_file_inode.ino = avail_ino;
			target_file_inode.valid = 1;
			target_file_inode.type = REG_FILE;
			target_file_inode.size = 0;
			target_file_inode.direct_ptr[0] = (superblock.d_start_blk + avail_block) * BLOCK_SIZE;

			// Step 6: Call writei() to write inode to disk
			writei(avail_ino, &target_file_inode);
			return 0;
		}

	}
	return -1;
}

static int rufs_open(const char *path, struct fuse_file_info *fi) {
	struct inode inode;
	// Step 1: Call get_node_by_path() to get inode from path
	if (get_node_by_path(path, 0, &inode)) {
		return 0;
	}
	// Step 2: If not find, return -1

	return -1;
}

static int rufs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode inode;
	if (!get_node_by_path(path, 0, &inode) || offset / (int) BLOCK_SIZE > 15) {
		return 0;
	}

	// Step 2: Based on size and offset, read its data blocks from disk
	int first_block_index = offset / ((int) BLOCK_SIZE);
	int last_block_index = (offset + size) / ((int) BLOCK_SIZE);
	last_block_index = last_block_index < 16 ? last_block_index : 15;

	if (!inode.direct_ptr[first_block_index]) {
		return 0;
	}

	// Step 3: copy the correct amount of data from offset to buffer
	char data_block[BLOCK_SIZE];
	bio_read(inode.direct_ptr[first_block_index] / BLOCK_SIZE, data_block);

	if (first_block_index == last_block_index) {
		memcpy(buffer, &data_block[offset], size);
		return size;
	}

	int bytes_copied = BLOCK_SIZE - offset;
	memcpy(buffer, &data_block[offset], bytes_copied);
	
	for (int i = first_block_index + 1; i < last_block_index; i++) {
		if (!inode.direct_ptr[i]) {
			return 0;
		} else {
			bio_read(inode.direct_ptr[i] / (int) BLOCK_SIZE, data_block);
			memcpy(buffer + bytes_copied, &data_block, BLOCK_SIZE);
			bytes_copied += BLOCK_SIZE;
		}
	}

	if (!inode.direct_ptr[last_block_index]) {
		return 0;
	}

	int remaining_size = size - bytes_copied;
	bio_read(inode.direct_ptr[last_block_index], data_block);
	memcpy(buffer + bytes_copied, &data_block, remaining_size);
	bytes_copied += remaining_size;

	// Note: this function should return the amount of bytes you copied to buffer
	return bytes_copied;
}

static int rufs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) { 
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk


	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode inode;
	if (!get_node_by_path(path, 0, &inode) || offset / (int) BLOCK_SIZE > 15) {
		return 0;
	}

	// Step 2: Based on size and offset, read its data blocks from disk
	int first_block_index = offset / (int) BLOCK_SIZE;
	int last_block_index = offset + size / (int) BLOCK_SIZE;
	last_block_index = last_block_index < 16 ? last_block_index : 15;

	char data_block[BLOCK_SIZE];
	int bytes_copied = BLOCK_SIZE - offset;
	if (!inode.direct_ptr[first_block_index]) {
		// Step 3: write the correct amount of data from offset to buffer

		int avail_block = get_avail_blkno();
		inode.direct_ptr[first_block_index] = (superblock.d_start_blk + avail_block) * BLOCK_SIZE;
		memcpy(&data_block[offset], buffer, bytes_copied);
		bio_write(inode.direct_ptr[first_block_index] / (int) BLOCK_SIZE, &data_block);

	}else{ //partially filled block of direct pointer
		bytes_copied = bytes_copied-sizeof(inode.direct_ptr[first_block_index]); 
		memcpy(&data_block[offset], buffer, bytes_copied);
		bio_write(inode.direct_ptr[first_block_index] / (int) BLOCK_SIZE, &data_block);	
	}

	if (first_block_index == last_block_index) {
		memcpy(&data_block[offset], buffer, size);
		return size;
	}

	for (int i = first_block_index + 1; i < last_block_index; i++) {
		if (!inode.direct_ptr[i]) {
			int avail_block = get_avail_blkno();
			inode.direct_ptr[i] = (superblock.d_start_blk + avail_block) * BLOCK_SIZE;
			char data_block[BLOCK_SIZE];
			memcpy(&data_block, buffer + bytes_copied, BLOCK_SIZE);
			bio_write(inode.direct_ptr[i] / (int) BLOCK_SIZE, &data_block);
			bytes_copied += BLOCK_SIZE;
		}
	}
	
	int remaining_size = size - bytes_copied;
	bytes_copied += remaining_size;
	if (!inode.direct_ptr[last_block_index]) {
		int avail_block = get_avail_blkno();
		inode.direct_ptr[last_block_index] = (superblock.d_start_blk + avail_block) * BLOCK_SIZE;
		char data_block[BLOCK_SIZE];
		memcpy(&data_block, buffer + bytes_copied, BLOCK_SIZE);
		bio_write(inode.direct_ptr[last_block_index] / (int) BLOCK_SIZE, &data_block);
	}




	// Note: this function should return the amount of bytes you copied to buffer
	return size;
}

static int rufs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char *lastSlash = strrchr(path, '/');
    const char *base = lastSlash ? lastSlash + 1 : path;

    char dir[lastSlash + 1 - path];
	memcpy(&dir, path, lastSlash + 1 - path);

	// Step 2: Call get_node_by_path() to get inode of target file
	struct inode target_file;
	if (!get_node_by_path(path, 0, &target_file)) {
		return -1;
	}

	// Step 3: Clear data block bitmap of target file
	int num_blocks = target_file.size / (int) BLOCK_SIZE + 1;

	for (int i = 0; i < num_blocks; i++) {
		unset_bitmap(data_bitmap, target_file.direct_ptr[i] / (int) BLOCK_SIZE);
	}

	char data_bitmap_block[BLOCK_SIZE];
	memcpy(data_bitmap_block, data_bitmap, superblock.max_dnum / 8);
	bio_write(superblock.d_bitmap_blk, data_bitmap_block);

	// Step 4: Clear inode bitmap and its data block
	unset_bitmap(inode_bitmap, target_file.ino);

	char inode_bitmap_block[BLOCK_SIZE];
	memcpy(inode_bitmap_block, inode_bitmap, superblock.max_inum / 8);
	bio_write(superblock.i_bitmap_blk, inode_bitmap_block);

	target_file.valid = 0;
	writei(target_file.ino, &target_file);

	struct inode parent_dir;
	// Step 5: Call get_node_by_path() to get inode of parent directory
	get_node_by_path(dir, 0, &parent_dir);

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory
	dir_remove(parent_dir, base, sizeof(base));

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

