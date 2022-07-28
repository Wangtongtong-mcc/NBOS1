#include "kernel.h"
#include "elf.h"
#include "memory.h"
#include "x86.h"
#include "lock.h"

extern struct lock disk_lock;

/* load用于加载磁盘上的用户进程到内存中
 * */
unsigned int load_process(unsigned char *destpos, unsigned int sector) {

	// 先加载elf_header
	struct elf_header *elfHeader = (struct elf_header *) (destpos + PAGESIZE);
	read_seg((unsigned char *) elfHeader, PAGESIZE, sector, 0);
	if (elfHeader->magic != ELF_MAGIC) {
		return -1;
	}

	// 再根据elf_header记载将各个段加载到destpos处
	struct program_header *programHeader = (struct program_header *) ((unsigned char *) elfHeader + elfHeader->e_phoff);

	for (int i = 0; i < elfHeader->e_phnum; programHeader++, i++) {
		if (programHeader->p_type == LOAD_SEG) {
			read_seg(destpos, programHeader->p_filesize, sector, programHeader->p_offset);
			destpos = destpos + programHeader->p_memsize;
		}
	}

	// 返回入口的虚拟地址
	return elfHeader->e_entry;
}


/* read_seg用于将指定磁盘位置，指定大小的块安装在指定内存处
 * 参数：target_location为内存位置，size为数据块在磁盘中的大小
 *      start_sector和offset分别为起始扇区和距起始的偏移字节
 * */
void read_seg(unsigned char * target_location, unsigned int size, unsigned int start_sector, unsigned int offset){
	// 读取磁盘只能以扇区为单位
	unsigned int sector = start_sector + offset/SECTOR_SIZE;                // 原数据所在扇区
	target_location = target_location - offset%SECTOR_SIZE;                 // 按扇区大小向下对齐
	unsigned char * target_end = target_location + size;


	for(;target_location < target_end;target_location += SECTOR_SIZE, sector++) {
		k_readsector(target_location, sector);
	}

}

/* k_operatesector用于操作磁盘
 * 参数：target_location为内存位置
 *      sector_number为扇区号码
 *      rwflag为读写标志
 * */
void k_operatesector(unsigned char* target_location, unsigned int sector_number, int rwflag){
	acquire(&disk_lock);

	// 先选择通道（Primary），并向该通道sector count（0x1f2）寄存器写入待写扇区数
	outb(IDE_SECTOR_COUNT_PORT,1);
	// 向该通道的三个LBA寄存器写入起始扇区号的低24位
	outb(IDE_SECTOR_NUMBER_PORT,(unsigned char)(sector_number & 0x000000ff));
	outb(IDE_CYLINDER_LOW_PORT,(unsigned char)((sector_number & 0x0000ff00)>>8));
	outb(IDE_CYLINDER_HIGH_PORT,(unsigned char)((sector_number & 0x00ff0000)>>16));
	// 向device寄存器中写入扇区号的24～27位，并设置第六位开启LBA模式，设置第四位，选择主盘或从盘
	outb(IDE_DEVICE_PORT,(unsigned char) (((sector_number & 0x0f0000) >> 24) | 0xe0));
	// 向command中写入命令（0x20为读， 0x30为写）
	unsigned short port = rwflag == READ_DISK_FLAG ? 0x20 : 0x30;
	outb(IDE_STATUS_PORT,port);

	unsigned char status;
	// 读取status，判断硬盘是否完成数据准备
	inb(IDE_STATUS_PORT,status);
	while ((status & 0x08) == 0) {
		inb(IDE_STATUS_PORT, status);
	}

	// 若硬盘已准备好，就开始写数据
	// data寄存器为16位，一次读一个字
	for (int i = 0; i < 256; i++) {
		unsigned short data;
		if (rwflag == READ_DISK_FLAG){
			inw(IDE_DATA_PORT, data);
			*((unsigned short *) target_location) = data;
		} else{
			data = *((unsigned short *) target_location);
			outw(IDE_DATA_PORT, data);
		}

		target_location += 2;
	}

	release(&disk_lock);
}


/* k_readsector用于将指定磁盘扇区，加载到指定内存处
 * 参数：target_location为内存位置，
 *      sector_number为扇区号
 * */
void k_readsector(unsigned char* target_location, unsigned int sector_number) {

	k_operatesector(target_location,sector_number,READ_DISK_FLAG);

}

/* k_writesector用于将指定内容，写回到磁盘指定扇区
 * 参数：target_location为内存位置，
 *      sector_number为扇区号
 * */
void k_writesector(unsigned char* target_location, unsigned int sector_number){

	k_operatesector(target_location,sector_number,WRITE_DISK_FLAG);
}


