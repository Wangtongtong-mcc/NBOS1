#include "kernel.h"

unsigned int pagedir [PAGE_STRUCT_ENTRIES] __attribute__ ((aligned (0x1000)));
unsigned int pagetables [256][PAGE_STRUCT_ENTRIES] __attribute__ ((aligned (0x1000)));
unsigned int stack[1024];
struct global_descriptor gdt[6];
struct gdtr_structure gdtr;

void main();

void entry(){


	// 1. 填充页目录
	unsigned int * pagedir_phyaddr = (unsigned int *)VIR_2_PHY((unsigned int)pagedir);
	unsigned int (*pagetables_phyaddr) [PAGE_STRUCT_ENTRIES] = (unsigned int (*) [PAGE_STRUCT_ENTRIES]) VIR_2_PHY((unsigned int)pagetables);

	//	unsigned int * pagetables_phyaddr = (unsigned int *)VIR_2_PHY((unsigned int)pagetables);

	for (int i = (1024-256); i < 1024; i++){

		//unsigned int * pagetable_phyaddr= pagetables_phyaddr[i-768];
		//unsigned int * pagetable_phyaddr = pagetables_phyaddr + (i-768) * PAGE_STRUCT_ENTRIES;

		unsigned int pde = (unsigned int)pagetables_phyaddr[i-768] + PAGE_S + PAGE_RW + PAGE_P;
		pagedir_phyaddr[i] = pde;

		for(int j = 0; j < 1024; j++){
			int phyaddr_base = ((i-768)<<22) + (j<<12);
			if(phyaddr_base > PHY_SIZE){
				goto done;
			}
			unsigned int pte = phyaddr_base + PAGE_P + PAGE_RW + PAGE_S;
			pagetables_phyaddr[i-768][j] = pte;
			//*(pagetables_phyaddr+ i*PAGE_STRUCT_ENTRIES +j) = pte;
		}
		done:
	}

	// 2.填充页表
//	for(int i = 0; i < 256; i++){
//		for(int j = 0; j < 1024; j++){
//			int phyaddr_base = (i<<24) + (j<<12);
//			if(phyaddr_base > PHY_SIZE){
//				goto done;
//			}
//			unsigned int pte = phyaddr_base + PAGE_P + PAGE_RW + PAGE_S;
//			pagetables_phyaddr[i][j] = pte;
//			//*(pagetables_phyaddr+ i*PAGE_STRUCT_ENTRIES +j) = pte;
//		}
//	}


	// 3.填充第一个pde
	pagedir_phyaddr[0] = pagedir_phyaddr[768];

	// 4.开启分页
	// 加载PDBR
	unsigned int cr3 = (unsigned int)pagedir_phyaddr | 0x00000000;
	asm("mov %%eax, %%cr3"::"a"(cr3));
	// 开启分页
	asm("mov %cr0, %eax; or $0x80000000, %eax; mov %eax,%cr0");

	// 5. 设置栈指针
	unsigned int stack_top = (unsigned int)stack + PAGESIZE;
	asm("mov %%eax, %%esp"::"a"(stack_top));

	// 6. 设置gdt
	gdt[0] = (struct global_descriptor){0,0,0,0,0};
	gdt[1] = (struct global_descriptor){0xffff,0x0000,0x00,0x9a,0xcf,0x00};
	gdt[2] = (struct global_descriptor){0xffff,0x0000,0x00,0x92,0xcf,0x00};
	gdt[3] = (struct global_descriptor){0xffff,0x0000,0x00,0xfa,0xcf,0x00};
	gdt[4] = (struct global_descriptor){0xffff,0x0000,0x00,0xf2,0xcf,0x00};

	gdtr.gdt_size = 6*8 - 1;
	gdtr.gdt_addr_low = (unsigned short)((unsigned int)gdt & 0x0000ffff);
	gdtr.gdt_addr_high = (unsigned short)(((unsigned int)gdt & 0xffff0000)>>16);

	asm("lgdt gdtr");

	// 7. 跳转到main
	asm("mov $main, %eax");
	asm("jmp %eax");
}