#ifndef _BOOT_LOADER_H
#define _BOOT_LOADER_H

#define KERNEL_SECTOR 6                             // 内核在磁盘中的起始扇区
#define PAGESIZE 1024
#define SECTOR_SIZE 512
#define VIR_2_PHY(x) ((x)-0xc0000000)


void read_seg(unsigned char * target_location, unsigned int size, unsigned int start_sector, unsigned int offset);
void read_sector(unsigned char * target_location, unsigned int sector);

#endif