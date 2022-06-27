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

	// 初始化中断
	init_interrupt();
	// 初始化8259A
	init_pic();
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
	// 中断初始化之后，可以打开中断
	// asm("sti");
	start_user_process();




	while (1) {

	}
}