#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

struct __attribute__((__packed__)) superblock {
	uint8_t signature[8];
	uint16_t totalBlocks;
	uint16_t rootDirectoryIndex;
	uint16_t dataBlockIndex;
	uint16_t dataBlockAmount;
	uint8_t numFATBlocks;
	uint8_t padding[4079];
};

struct __attribute__((__packed__)) rootDirectory {
	uint8_t fileName[FS_FILENAME_LEN];
	uint32_t fileSize;
	uint16_t firstIndex;
	uint8_t padding[10];
};

struct __attribute__((__packed__)) FAT {
	uint16_t *blocks;
};

struct __attribute__((__packed__)) fileSystem {
	struct superblock superblock;
	struct FAT FAT;
	struct rootDirectory rootDirectory[FS_FILE_MAX_COUNT];
	int currentlyMounted;
};



// struct __attribute__((__packed__)) FATNode {
// 	FATNode_t prev;
// 	FATNode_t next;
// 	void* data;
// };


struct fileSystem fileSystem;

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */

	int openStatus = block_disk_open(diskname);

	if (openStatus == -1) {
		return openStatus;
	}

	fileSystem.currentlyMounted = 0;

	int readStatus = block_read(0, &fileSystem.superblock);

	if (readStatus == -1) { 
		return readStatus;
	}

	fileSystem.FAT.blocks = calloc(BLOCK_SIZE * fileSystem.superblock.numFATBlocks, sizeof(uint16_t));

	int signatureStatus = strcmp("ECS150FS", (char*)fileSystem.superblock.signature);

	if (signatureStatus == -1) {
		return signatureStatus;
	}

	int diskCountStatus = block_disk_count();

	if (fileSystem.superblock.totalBlocks != diskCountStatus) {
		return -1;
	}

	fileSystem.currentlyMounted = 1;

	// fileSystem = calloc(1, sizeof(struct fileSystem));
	
	for(int i = 1; i <= fileSystem.superblock.numFATBlocks; i++){

		block_read(i, &fileSystem.FAT.blocks[i - 1]);
	}

	block_read(fileSystem.superblock.rootDirectoryIndex, &fileSystem.rootDirectory);

	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */

	// FATNode_t newFATNode = calloc(1, sizeof(struct FATNode));

	// newFATNode = fileSystem.FAT.head;

	if (fileSystem.currentlyMounted == 0) {
		return -1;
	}

	int readStatus = block_read(0, &fileSystem.superblock);

	if (readStatus == -1) {
		return -1;
	}

	int index = 1;

	while (index <= fileSystem.superblock.numFATBlocks) {

		block_write(index, &fileSystem.FAT.blocks[index - 1]);

		index++;
	}

	block_write(fileSystem.superblock.numFATBlocks + 1, &fileSystem.rootDirectory);

	block_disk_close();

	fileSystem.currentlyMounted = 0;

	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */

	if (fileSystem.currentlyMounted == 0) {
		return -1;
	}

	int emptyFAT = 0;

	int emptyRootDir = 0;

	int index = 0;

	for (int i = 0; i < fileSystem.superblock.dataBlockAmount; i++){

		if (fileSystem.FAT.blocks[index] == 0) {

			emptyFAT++;
		
		}
				
		index++;

	}

	for (index = 0; index < FS_FILE_MAX_COUNT; index++) {

		if (fileSystem.rootDirectory[index].fileName[0] == 0) {
		
			emptyRootDir++;
		
		}
	
	}

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", fileSystem.superblock.totalBlocks);
	printf("fat_blk_count=%d\n", fileSystem.superblock.numFATBlocks);
	printf("rdir_blk=%d\n", fileSystem.superblock.rootDirectoryIndex);
	printf("data_blk=%d\n", fileSystem.superblock.dataBlockIndex);
	printf("data_blk_count=%d\n", fileSystem.superblock.dataBlockAmount);
	printf("fat_free_ratio=%d/%d\n", emptyFAT, fileSystem.superblock.dataBlockAmount);
	printf("rdir_free_ratio=%d/%d\n", emptyRootDir, FS_FILE_MAX_COUNT);

	return 0;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */

	// for (int index = 0; index < FS_FILE_MAX_COUNT; index++) {
		
	// 	int comparison = strcmp((char*)fileSystem.rootDirectory[index].fileName, NULL);

	// 	if (comparison == 0) {

	// 		memcpy(fileSystem.rootDirectory[index].fileName.fileName, filename, FS_FILENAME_LEN);

	// 		fileSystem.rootDirectory[index].fileSize = 0;
			
	// 		fileSystem.rootDirectory[index].firstIndex = 0xFFFF;

	// 	}

	// }

	(void)filename;
	
	return 0;

}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */

	(void)filename;
	return 0;

}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */

	(void)filename;
	return 0;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	(void)fd;
	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	(void)fd;
	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	(void)fd;
	(void)offset;
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	(void)fd;
	(void)buf;
	(void)count;
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	(void)fd;
	(void)buf;
	(void)count;
	return 0;
}

