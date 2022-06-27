#ifndef _BOOT_LOADER_H
#define _BOOT_LOADER_H

void read_seg(unsigned char * target_location, unsigned int size, unsigned int start_sector, unsigned int offset);
void read_sector(unsigned char * target_location, unsigned int sector);

#endif