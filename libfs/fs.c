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

struct __attribute__((__packed__)) fileDescriptor {
	uint8_t fileName[FS_FILENAME_LEN];
	uint16_t index;
	size_t offset;
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
	struct fileDescriptor fileDescriptor[FS_OPEN_MAX_COUNT];
	int filesOpen;
	int currentlyMounted;
};

struct __attribute__((__packed__)) bufferStruct {
	int offset;
	int bytesRead;
	int fileSize;
	uint16_t firstIndex;
	uint16_t currentIndex;
	uint16_t currentBlock;
	void* bounceBuffer;
};

struct fileSystem fileSystem;
struct bufferStruct writeBuffer;
struct bufferStruct readBuffer;

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */

	int openStatus = block_disk_open(diskname);

	if (openStatus == -1) {
		return openStatus;
	}

	fileSystem.currentlyMounted = -1;

	fileSystem.filesOpen = 0;

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

	fileSystem.currentlyMounted = 0;

	// fileSystem = calloc(1, sizeof(struct fileSystem));

	int index = 0;

	while (index < fileSystem.superblock.numFATBlocks) {
		
		block_read(index + 1, &fileSystem.FAT.blocks[index]);

		index++;
	}

	block_read(fileSystem.superblock.rootDirectoryIndex, &fileSystem.rootDirectory);

	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */

	// FATNode_t newFATNode = calloc(1, sizeof(struct FATNode));

	// newFATNode = fileSystem.FAT.head;

	int mountStatus = fileSystem.currentlyMounted;

	if (mountStatus == -1) {
		return mountStatus;
	}

	int readStatus = block_read(0, &fileSystem.superblock);

	if (readStatus == -1) {
		return readStatus;
	}

	int index = 1;

	while (index <= fileSystem.superblock.numFATBlocks) {

		block_write(index, &fileSystem.FAT.blocks[index - 1]);

		index++;
	}

	block_write(fileSystem.superblock.numFATBlocks + 1, &fileSystem.rootDirectory);

	block_disk_close();

	fileSystem.currentlyMounted = -1;

	free(fileSystem.FAT.blocks);

	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */

	int mountStatus = fileSystem.currentlyMounted;

	if (mountStatus == -1) {
		return mountStatus;
	}

	int emptyFAT = 0;

	int emptyRootDir = 0;

	int index = 0;

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", fileSystem.superblock.totalBlocks);
	printf("fat_blk_count=%d\n", fileSystem.superblock.numFATBlocks);
	printf("rdir_blk=%d\n", fileSystem.superblock.rootDirectoryIndex);
	printf("data_blk=%d\n", fileSystem.superblock.dataBlockIndex);
	printf("data_blk_count=%d\n", fileSystem.superblock.dataBlockAmount);

	while (index < fileSystem.superblock.dataBlockAmount){

		if (fileSystem.FAT.blocks[index] == 0) {

			emptyFAT++;
		
		}
				
		index++;

	}

	printf("fat_free_ratio=%d/%d\n", emptyFAT, fileSystem.superblock.dataBlockAmount);

	index = 0;

	while (index < FS_FILE_MAX_COUNT) {

		if (fileSystem.rootDirectory[index].fileName[0] == 0) {
		
			emptyRootDir++;
		
		}
		
		index++;
	
	}

	printf("rdir_free_ratio=%d/%d\n", emptyRootDir, FS_FILE_MAX_COUNT);

	return 0;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */

	int mountStatus = fileSystem.currentlyMounted;

	if (mountStatus == -1) {
		return mountStatus;
	}

	int nameLength = strlen(filename);

	if (nameLength > FS_FILENAME_LEN) {
		return -1;
	}

	int diskCountStatus = block_disk_count();

	if (diskCountStatus == -1) {
		return diskCountStatus;
	}

	int index = 0;

	while (index < FS_FILE_MAX_COUNT) {
		
		int comparison = strcmp((char*)fileSystem.rootDirectory[index].fileName, filename);

		if (comparison == 0) {

			return -1;

		}

		else if (*fileSystem.rootDirectory[index].fileName == '\0') {

			strcpy((char*)fileSystem.rootDirectory[index].fileName, filename);
			fileSystem.rootDirectory[index].fileSize = 0;
			fileSystem.rootDirectory[index].firstIndex = 0xFFFF;
			block_write(fileSystem.superblock.rootDirectoryIndex, &fileSystem.rootDirectory);
			return 0;
		}

		else {

			index++;
		}

	}
	
	return -1;

}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */

	int nameLength = strlen(filename);

	if (nameLength > FS_FILENAME_LEN) {
		return -1;
	}

	int diskCountStatus = block_disk_count();

	if (diskCountStatus == -1) {
		return diskCountStatus;
	}

	int index = 0;

	int comparison = -1;

	while (index < FS_FILE_MAX_COUNT) {

		comparison = strcmp((char*)fileSystem.rootDirectory[index].fileName, filename);

		if (comparison == 0) {
			comparison = 99;
			break;
		}

		index++;
	}

	if (comparison != 99) {
		return -1;
	}

	uint16_t tempIndex;

	while (fileSystem.rootDirectory[index].firstIndex != 0xFFFF) {
		
		tempIndex = fileSystem.FAT.blocks[fileSystem.rootDirectory[index].firstIndex];

		fileSystem.FAT.blocks[fileSystem.rootDirectory[index].firstIndex] = 0;

		fileSystem.rootDirectory[index].firstIndex = tempIndex;

	}

	*fileSystem.rootDirectory[index].fileName = '\0';

	fileSystem.rootDirectory[index].firstIndex = 0xFFFF;
	
	fileSystem.rootDirectory[index].fileSize = 0;
	
	block_write(fileSystem.superblock.rootDirectoryIndex, &fileSystem.rootDirectory);

	return 0;

}

int fs_ls(void)
{
	/* TODO: Phase 2 */

	int index = 0;

	printf("FS Ls\n");

	while (index < FS_FILE_MAX_COUNT) {

		if (*fileSystem.rootDirectory[index].fileName != '\0') {
			printf("file: %s, ", fileSystem.rootDirectory[index].fileName);
			printf("size: %d, ", fileSystem.rootDirectory[index].fileSize);
			printf("data_blk: %d\n", fileSystem.rootDirectory[index].firstIndex);
		}

		index++;

	}

	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */

	int mountStatus = fileSystem.currentlyMounted;

	if (mountStatus == -1) {
		return mountStatus;
	}

	int nameLength = strlen(filename);

	if (nameLength > FS_FILENAME_LEN) {
		return -1;
	}

	int diskCountStatus = block_disk_count();

	if (diskCountStatus == -1) {
		return diskCountStatus;
	}

	int numberOpen = fileSystem.filesOpen;

	if (numberOpen == FS_OPEN_MAX_COUNT) {
		return -1;
	}

	int index = 0;

	int comparison = -1;

	while (index < FS_FILE_MAX_COUNT) {

		comparison = strcmp((char*)fileSystem.rootDirectory[index].fileName, filename);

		if (comparison == 0) {
			comparison = 99;
			break;
		}

		index++;
	}

	if (comparison != 99) {
		return -1;
	}

	int matchLocation = index;

	index = 0;

	while (index < FS_FILE_MAX_COUNT) {

		if (*fileSystem.rootDirectory[index].fileName == '\0') {
			break;
		}

		index++;
	}

	fileSystem.filesOpen++;

	strcpy((char*)fileSystem.fileDescriptor[index].fileName, filename);

	fileSystem.fileDescriptor[index].offset = 0;

	fileSystem.fileDescriptor[index].index = matchLocation;

	return index;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */

	int mountStatus = fileSystem.currentlyMounted;

	if (mountStatus == -1) {
		return mountStatus;
	}

	if (fd < 0 || fd > 31) {
		return -1;
	}
	
	if (*fileSystem.fileDescriptor[fd].fileName == '\0') {
		return -1;
	}

	fileSystem.filesOpen--;

	*fileSystem.fileDescriptor[fd].fileName = '\0';

	fileSystem.fileDescriptor[fd].offset = 0;

	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */

	int mountStatus = fileSystem.currentlyMounted;

	if (mountStatus == -1) {
		return mountStatus;
	}

	if (fd < 0 || fd > 31) {
		return -1;
	}

	if (*fileSystem.fileDescriptor[fd].fileName == '\0') {
		return -1;
	}

	return fileSystem.rootDirectory[fileSystem.fileDescriptor[fd].index].fileSize;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */

	int mountStatus = fileSystem.currentlyMounted;

	if (mountStatus == -1) {
		return mountStatus;
	}

	if (fd < 0 || fd > 31) {
		return -1;
	}

	if (*fileSystem.fileDescriptor[fd].fileName == '\0') {
		return -1;
	}

	fileSystem.fileDescriptor[fd].offset = offset;
	return 0;
}

uint16_t fs_offset_index(struct bufferStruct bufferStruct) {

	uint16_t index = BLOCK_SIZE;

	index--;
	
	while (index < (int)bufferStruct.offset) {

		bufferStruct.currentIndex = fileSystem.FAT.blocks[bufferStruct.currentIndex];

		if (bufferStruct.currentIndex == 0xFFFF) {

			break;
		}

		index += BLOCK_SIZE;
	}

	return bufferStruct.currentIndex;
}

struct bufferStruct bufferInitializaton (struct bufferStruct bufferStruct, int fd) {

	bufferStruct.bounceBuffer = calloc(1, BLOCK_SIZE);

	bufferStruct.bytesRead = 0;
	
	bufferStruct.firstIndex = fileSystem.fileDescriptor[fd].index;

	bufferStruct.offset = fileSystem.fileDescriptor[fd].offset;

	bufferStruct.fileSize = fs_stat(fd);

	bufferStruct.currentIndex = fileSystem.rootDirectory[readBuffer.firstIndex].firstIndex;

	return bufferStruct;
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

	int mountStatus = fileSystem.currentlyMounted;

	if (mountStatus == -1) {
		return mountStatus;
	}

	if (fd < 0 || fd > 31) {
		
		return -1;
	}

	if (*fileSystem.fileDescriptor[fd].fileName == '\0') {

		return -1;
	}

	if (buf == NULL) {

		return -1;
	}

	readBuffer = bufferInitializaton(readBuffer, fd);

	block_read(fs_offset_index(readBuffer) + fileSystem.superblock.dataBlockIndex, readBuffer.bounceBuffer);

	size_t index = 0;

	int offsetValue = 0;

	while (readBuffer.offset >= BLOCK_SIZE) {

		readBuffer.offset -= BLOCK_SIZE;

		offsetValue++;
	}

	while (index < count) {

		if (readBuffer.fileSize <= (int)readBuffer.offset) {

			break;
		}

		if (readBuffer.offset == BLOCK_SIZE) {

			int nextIndex = fs_offset_index(readBuffer) + fileSystem.superblock.dataBlockIndex;

			int trueIndex = nextIndex - fileSystem.superblock.dataBlockIndex;

			if (trueIndex == 0xFFFF) {
				
				break;
			}

			block_read((size_t)nextIndex, readBuffer.bounceBuffer);

			readBuffer.offset = 0;

			offsetValue++;
			
		}

		readBuffer.offset++;

		readBuffer.bytesRead++;

		index++;
	}

	memcpy(buf, readBuffer.bounceBuffer, readBuffer.bytesRead);

	fileSystem.fileDescriptor[fd].offset = readBuffer.offset + (offsetValue * BLOCK_SIZE);

	free(readBuffer.bounceBuffer);

	return readBuffer.bytesRead;
}

