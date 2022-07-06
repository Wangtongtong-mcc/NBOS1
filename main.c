#include "interrupt.h"
#include "process.h"
#include "file.h"
#include "memory.h"
#include "debug.h"
#include "syscall.h"



/* main.c为内核的主要内容
 * */
unsigned int pagedir [PAGE_STRUCT_ENTRIES] __attribute__ ((aligned (0x1000)));
unsigned int pagetable[PAGE_STRUCT_ENTRIES] __attribute__ ((aligned (0x1000)));
struct cpu mycpu;




void main(void){

	clear_screen();


	print("Kernel is working!\n");

	// 初始化8259A,其中包括对时钟芯片的初始化
	init_pic();
	// 初始化中断描述符表
	init_interrupt();
	// 初始化内存结构
	init_memory();
	// 初始化tss
	init_tss();
	// 初始化进程控制块
	init_pcb();
	// 初始化文件系统
	init_file();
	// 创建第一个用户进程shell
	first_user_process();
	// 开启第一个用户进程
	start_user_process();


	while (1) {

	}
}