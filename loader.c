#include "loader.h"
#include "elf.h"
#include "memory.h"

/* loader.c由mbr加载到0x7e00处
 * 用于加载内核，并跳转到内核执行
 * */


void loader_main (void){


	// 加载数据段寄存器
	asm("mov $0x0010, %ax;mov %ax, %ds;mov %ax, %ss; mov %ax, %es");

	// 先将内核文件的elf header读到缓冲区(内存0x9000的地方)
	struct elf_header * eh = (struct elf_header *)0x9000;
	read_seg((unsigned char*) eh, PAGESIZE, KERNEL_SECTOR, 0);
	if(eh->magic != ELF_MAGIC){
		return;
	}

	// 从elf_header中获取program_header的指针pb，循环加载各个段
	struct program_header * ph = (struct program_header *)((unsigned char *)eh + eh->e_phoff);

	for(int i = 0 ; i < eh->e_phnum; ph++,i++){
		if (ph->p_type == LOAD_SEG){
			unsigned char * pa = (unsigned char *)(VIR_2_PHY(ph->p_vaddr));
			read_seg(pa,ph->p_filesize,KERNEL_SECTOR,ph->p_offset);
		}
	}

	// 获取内核入口地址（elf文件中为虚拟地址), 并调用
	void (* entry) (void) = (void (*)(void))VIR_2_PHY(eh->e_entry);
	entry();
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

	for(;target_location < target_end;target_location += SECTOR_SIZE, sector++){
		read_sector(target_location,sector);
	}

}




