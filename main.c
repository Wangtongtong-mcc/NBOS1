#include "interrupt.h"
#include "process.h"
#include "file.h"
#include "memory.h"
#include "debug.h"
#include "syscall.h"
#include "lock.h"
#include "unistd.h"
#include "kernel.h"



/* main.c为内核的主要内容
 * */
struct cpu mycpu;


void switch_to_user(){

	// 0. 先保存本函数的返回地址
	unsigned int iret_addr;
	asm("movl +4(%%ebp), %%eax"\
	:"=a"(iret_addr));

	// 1. 申请一个页作为页表
	struct page_table_t * page_table_ptr = (struct page_table_t *)alloc_page();

	// 2. 将内核空间第一个页表复制到新页中

	// 2.1 获取第一个页表对应的页目录项地址
	struct page_dir_item_t * base_page_dir_item_ptr = (struct page_dir_item_t *)mycpu.current_process->pagedir;

	// 2.2 获取内核空间第一个页表的地址
	struct page_table_t * first_kernel_page_table_ptr = (struct page_table_t *)PHY_2_VIR((base_page_dir_item_ptr->address)<<12);

	// 2.3 复制到新页中
	memcopy((unsigned char *)first_kernel_page_table_ptr,(unsigned char *)page_table_ptr,PAGESIZE);

	// 3. 遍历页表的页表项，将每个页表项的权限修改为PAGE_PRIVILEGE_USER
	struct page_table_item_t * base_page_table_item_ptr = (struct page_table_item_t *)page_table_ptr;
	for(int i = 0; i < 1024; i++){
		(base_page_table_item_ptr + i)->privilege = PAGE_PRIVILEGE_USER;
	}

	// 4. 修改页目录第一个页目录项，将其地址修改为新的页表地址，将权限修改为PAGE_PRIVILEGE_USER
	generate_page_dir_item(base_page_dir_item_ptr,(struct page_table_t*)VIR_2_PHY((unsigned int)page_table_ptr),PAGE_PRIVILEGE_USER);

	// 5. 申请一个页作为0号用户进程的栈
	struct page_t * stack_page_ptr = (struct page_t *)alloc_page();
	page_map((struct page_t *)(USER_STACK_START & 0xfffff000),(struct page_t *)VIR_2_PHY((unsigned int)stack_page_ptr),PAGE_PRIVILEGE_USER);


	// 6. 将用户态iret参数压栈，分别压入ss , esp, eflags, cs, eip
	// 6.1 先获取当前eflags
	unsigned int eflags;
	asm("pushf; mov (%%esp), %%eax; popf"\
	:"=a"(eflags));

	// 6.2 获取当前的esp
	unsigned int esp;
	asm("movl %%esp, %%eax;"\
	:"=a"(esp));

	// 6.3 构造当前iret的参数
	struct iret_param * iret_param = (struct iret_param *)(esp - sizeof(struct iret_param));

	iret_param->cs = UCODE_SELECTOR;
	iret_param->ss = UDATA_SELECTOR;
	iret_param->esp = USER_STACK_START;
	iret_param->eflags = eflags;
	iret_param->eip = iret_addr -  0xc0000000;

	// 6.4 设置esp指针
	unsigned int iret_param_size = sizeof(struct iret_param);
	asm("sub %%eax, %%esp"\
	::"a"(iret_param_size));


	// 7. 修改数据段寄存器
	unsigned int data_selector = UDATA_SELECTOR;
	asm("movw %%ax, %%ds; movw %%ax, %%es"\
	::"a"(data_selector));

	// 8. 执行iret
	asm("iret");
}

int main(void){

	clear_screen();

	print("Kernel is working!\n");

	// 初始化内存结构
	init_memory();
	// 初始化8259A,其中包括对时钟芯片的初始化
	init_pic();
	// 初始化中断描述符表
	init_interrupt();
	// 初始化tss
	init_tss();
	// 初始化文件系统
	init_file();
	// 初始化进程控制块
	init_pcb();
	asm("sti");
	// 切换到用户态
	switch_to_user();

	// 现在已经切换到用户态，进程号为0，fork shell进程
	if(fork() == 0){
		execve("/shell",NULL,NULL);
	}



	while (1) {

	}
	return 0;
}