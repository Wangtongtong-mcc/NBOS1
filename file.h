#ifndef _KERNEL_FILE_H
#define _KERNEL_FILE_H

#include "kernel.h"

void init_file(void);

int isOpen(unsigned int fd, struct pcb * p);
int haveAccess(struct open_file * file, int flag);
int getMinfd(struct pcb * p);
struct open_file * setOpenFileList(int oflag,struct inode * iptr);
void delFile(struct open_file * ofptr);
int allocInode(struct inode * iptr);
int seekBaseDirectory(unsigned char *path,struct inode * base_iptr);
int allocBlock(struct inode *iptr);

void read_inode(unsigned int inode_block_no, unsigned int inode_offset, struct inode * iptr);

int writeDirectory(struct inode *base_inode,unsigned char *path,struct inode *iptr);
int writeInode(struct inode * iptr);

int writeFile(struct open_file * file,struct inode *iptr,void * ptr, unsigned int size);

int seekFileInode(char *path, struct inode *iptr);

int readFile(struct open_file * file,struct inode *iptr, void * ptr, unsigned int size);

int read_file_at_offset(struct inode *iptr, int offset, unsigned char * buf, unsigned int size);

#endif