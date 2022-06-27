#include "kernel.h"
#include "memory.h"
#include "print.h"

struct task_struct tss;
extern unsigned int pagedir[PAGE_STRUCT_ENTRIES];                   // 内核页目录目录
extern struct global_descriptor gdt;                                // gdt地址
struct global_descriptor tss_descriptor;
extern struct cpu mycpu;

/* init_tss用于初始化tss结构，注册到gdt中，并加载tr寄存器
 * */
void init_tss(void){
	print("Initial TSS!\n");

	// 首先填充内核tss结构
	tss.cr3 = &pagedir;
	tss.ss0 = KDATA_SELECTOR;
	tss.cs = KCODE_SELECTOR;
	tss.ds = KDATA_SELECTOR;
	tss.ds = tss.es = tss.fs = tss.gs = tss.ds ;
	tss.esp0 = (unsigned int)STACK_TOP;

	// 注册tss描述符到gdt中
	unsigned int tss_base = (unsigned int) (&tss);

	tss_descriptor.base_low = (unsigned short)(tss_base & 0x0000ffff);
	tss_descriptor.limit = (unsigned short)((sizeof(tss) - 1) & 0x0000ffff);
	tss_descriptor.base_mid = (unsigned char)((tss_base & 0x00ff0000)>>16);
	tss_descriptor.attr_low = (unsigned char)TSS_ATTR_LOW;
	tss_descriptor.attr_high = (unsigned char)TSS_ATTR_HIGH;
	tss_descriptor.base_high = (unsigned char)((tss_base & 0xff000000)>>24);


	unsigned int tss_index = (TSS_SELECTOR & 0xf8)>>3;

	struct global_descriptor * tss_pos_in_gdt = (&gdt)+ tss_index;
	*tss_pos_in_gdt = tss_descriptor;


	// 将当前任务控制块装配到cpu结构中
	mycpu.current_task = &tss;

	// 加载tr寄存器
	asm("ltr %w0": :"r"(TSS_SELECTOR));

	print("Init_tss done!\n");

}