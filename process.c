#include "process.h"
#include "memory.h"
#include "print.h"
#include "elf.h"

extern unsigned int pagedir[PAGE_STRUCT_ENTRIES];                  // 内核页目录目录
struct pcb pcblist[PCB_MAX];                                       // pcb链表
unsigned int nextpid = 1;                                          // 下一个进程id
extern struct cpu mycpu;
extern struct open_file fileList[FILE_MAX_INSYS];
extern unsigned int kvir_next_addr;

/* init_pcb为用于初始化进程控制块
 * */
void init_pcb(void){

	print("Initial PCB!\n");

	for(int i = 0; i < PCB_MAX; i++){
		struct pcb * p = &pcblist[i];
		// 每个进程的虚拟地址从0开始
		p->next_vir_addr = 0;
		// 每个进程的初始状态均为FREE
		p->pstate = FREE;
		// 每个进程可最多打开FILE_MAX_InPROCESS个文件
		// 初始时，每个文件均为CLOSE状态，文件指针均为NULL
		for(int j = 0; j < FILE_MAX_InPROCESS; j++){
			p->fdlist[j].ofptr = NULL;
			p->fdlist[j].open_flag = CLOSE;
		}
	}

	// 进程0为内核
	pcblist[0].pid = 0;
	pcblist[0].pstate = RUNNING;
	pcblist[0].pagedir = &pagedir[0];
	pcblist[0].next_vir_addr = kvir_next_addr;

	print("Initial_pcb Done!\n");

}

/* first_user_process为创建第一个用户进程
 * */
void first_user_process(void){

	print("Create first user process!\n");

	// 首先，创建一个新进程
	struct pcb * p = create_process();
	// 进程为shell(为简单，一个用户进程最大为PAGESIZE)
	p->sector = SHELL_SECTOR;
	p->size = PAGESIZE;

	// 将进程先加载到内核缓冲区处, 获取该进程的入口地址
	unsigned int pentry = load_process((unsigned char *)BUFFER_START,p->sector);

	// 将内核地址空间所对应的页目录内容复制到进程页目录，则该进程可共享内核地址空间
	memcopy((unsigned char *)(&pagedir)+0xc00, (unsigned char *)p->pagedir+0xc00, PAGESIZE-0xc00);

	// 填充该进程的上下文
	p->ctx.cs = UCODE_SELECTOR;
	p->ctx.ds = UDATA_SELECTOR;
	p->ctx.es = p->ctx.ds;
	p->ctx.ss = p->ctx.ds;
	p->ctx.esp = PAGESIZE - 16;
	p->ctx.eip = pentry;

	// 将该进程的状态改为可运行
	p->pstate = RUNNABLE;

	print("Successfully created the first user process!\n");

}

/* create_process为创建进程
 * */
struct pcb * create_process(void){

	struct pcb * p;
	// 从进程1开始寻找空闲pcb，使p指向它
	for(int i = 1; i < PCB_MAX; i++){
		if (pcblist[i].pstate == FREE){
			p = &(pcblist[i]);
			break;
		}
	}

	// 设置进程初始状态及pid
	p->pstate = BORN;
	p->pid = nextpid;
	nextpid++;
	// 为该进程分配一个页作为其页目录
	p->pagedir = kalloc();
	// 为该进程打开标准输入，标准输出，标准错误
	for(int i = 0; i < 3; i++){
		p->fdlist[i].open_flag = OPEN;
	}
	// 为标准输入，标准输出，标准错误指向正确的文件表项
	p->fdlist[STDIN_FILENO].ofptr = &(fileList[STDIN_FILENO]);
	p->fdlist[STDOUT_FILENO].ofptr = &(fileList[STDOUT_FILENO]);
	p->fdlist[STDERR_FILENO].ofptr = &(fileList[STDERR_FILENO]);

	return p;
}

/* start_user_process为开启/切换进程
 * */
void start_user_process(void){

	print("Switch to the first user process!\n");

	// 找出pcb链表中可运行的进程
	struct pcb * p;
	for(int i = 0; i < PCB_MAX; i++){
		if (pcblist[i].pstate == RUNNABLE){
			p = &(pcblist[i]);
			break;
		}
	}
	// 设置cpu当前进程
	mycpu.current_process = p;
	// 切换页目录(PCD=PWT=0)
	unsigned int dir = VIR_2_PHY((unsigned int )p->pagedir);
	asm("mov %%eax, %%cr3"::"a"(dir));
	// 将用户进程复制到用户地址空间（虚拟地址从0x00000000开始，物理分配UFREE_START之上的页）
	unsigned int * user = ualloc(&(p->next_vir_addr),p->pagedir);
	memmov((unsigned char *)BUFFER_START, (unsigned char *)user,(unsigned int)PAGESIZE);
	// 切换进程状态
	p->pstate = RUNNING;
	// 切换上下文
	switchktou(&(pcblist[0].ctx),&(p->ctx));
	// 返回内核后，首先要



}






/* getPid为用于获取新的进程id
 * */
int getPid (void){

	int pid, i;
	// 取pid
	reget:
	pid = nextpid;
	nextpid++;
	// 若pid超出最大值，从1开始重新取
	if (pid > PID_MAX) pid = nextpid = 1;
	// 并且必须保证当前pid没有被系统内的进程占用
	for (i = 0; i < PCB_MAX; i++){
		if (pcblist[i].pid == pid ) goto reget;
	}

	// 如果找完全部pcb没有发现重复的pid,则可直接返回
	if(i == PCB_MAX && pid < PID_MAX){
		return pid;
	} else{
		return -1;
	}
}

/* allocProcess用于寻找一个空闲的pcb并返回
 */
struct pcb * allocProcess(void){


	// 取pid
	int pid = getPid();
	if (pid == -1){
		return NULL;
	}

	struct pcb * p = NULL;
	// 寻找一个空闲的进程控制块
	for(int i = 0; i < PCB_MAX;i++){
		if (pcblist[i].pstate == FREE){
			p = &(pcblist[i]);
			break;
		}
	}
	// 设置进程初始状态
	p->pstate = RUNNABLE;
	// 设置新进程pid
	p->pid = pid;
	p->father_pid = mycpu.current_process->pid;

	return p;

}

/* copyContext用于复制上下文
 * father为原进程的上下文，存于栈中
 * p为新进程
 */
int copyContext(struct intr_context *father,struct pcb *p){

	// 设置新进程的上下文
	// ss , esp, eflags, cs, eip为当前进程调用fork后压入栈中的
	// syscall_handler分别压入ds,es,fs,gs,all
	// eax需设置为0,这样子进程的返回结果就是0

	// 通用寄存器
	p->ctx.eax = 0;
	p->ctx.ebx = father->ebx;
	p->ctx.ecx = father->ecx;
	p->ctx.edx = father->edx;
	p->ctx.edi = father->edi;
	p->ctx.esi = father->esi;
	p->ctx.ebp = father->ebp;
	p->ctx.esp = father->esp;

	// 段寄存器
	p->ctx.ds = father->ds;
	p->ctx.es = father->es;
	p->ctx.fs = father->fs;
	p->ctx.gs = father->gs;
	p->ctx.cs = father->cs;
	p->ctx.ss = father->ss;

	// eip
	p->ctx.eip = father->eip;

	// 标志寄存器
	p->ctx.eflags = father->eflags;

	return 0;


}



/* readSegment用于将in对应的文件中的programHeader对应的段加载到内存location处
 */
int readSegment(struct inode *in, struct program_header *programHeader, unsigned char * location){

	unsigned char * p = location;
	// 取本段在文件中的偏移量
	unsigned int segstart_offset = programHeader->p_offset;
	unsigned int segend_offset = segstart_offset + programHeader->p_filesize;

	// 读入本段
	while (segstart_offset < segend_offset){
		unsigned char arr[512];
		// segstart_offset对应的block_no
		unsigned int block = in->block_no[segstart_offset / BLOCK_SIZE];
		// 本block还有多少字节待读入
		unsigned int left = BLOCK_SIZE - segstart_offset % BLOCK_SIZE;
		// 读入本block
		k_readsector(arr,block+SUPER_BLOCK_SECTOR);
		// 将所需内容复制到location处
		memcopy(arr,p,left);
		// 更新offset和当前目的指针
		segstart_offset +=left;
		p += left;
	}

	return 0;
}



/* loadProcess用于加载进程到内核缓存地址BUFFER_START处
 * in为进程在文件系统中对应的inode
 * location为将进程加载到内核的该虚拟地址处
 */
int loadProcess(struct inode * in){

	// 1. 首先加载进程的elf_header
	// 取文件前四个block，保证可读到完整的elf_header，program_header，将程序的这些元数据读到内核缓存的第二个页面
	struct elf_header *elfHeader = (struct elf_header *) (BUFFER_START + PAGESIZE);
	for(int i = 0; i < 4; i++){
		unsigned int block = in->block_no[i];
		if(block == 0){
			return -1;
		}
		k_readsector((unsigned char*)(BUFFER_START + PAGESIZE + i*BLOCK_SIZE ),block + SUPER_BLOCK_SECTOR);
	}
	if (elfHeader->magic != ELF_MAGIC) {
		return -1;
	}

	// 2. 根据elf_header中的信息，将各程序段加载到缓存处
	// firstVir为第一个可加载段的虚拟地址，loadseg为科技在段的数量
	unsigned int firstVir;
	int loadseg = 0;
	struct program_header * programHeader = (struct program_header *) ((unsigned char *) elfHeader + elfHeader->e_phoff);
	for (int j = 0; j < elfHeader->e_phnum; programHeader++, j++) {
		// 只有类型为LOAD_SEG的段才需要加载到内存中
		if(programHeader->p_type == LOAD_SEG){
			loadseg++;
			// 需确认第一个段的虚拟地址，之后加载时，才能确定各个段的相对位置
			if(loadseg == 1){
				firstVir = programHeader->p_vaddr;
			}
			// 从内存中加载本段
			unsigned char *location = (unsigned char *)(programHeader->p_vaddr - firstVir + BUFFER_START);
			readSegment(in, programHeader, location);
		}
	}

	// 3. 返回入口地址
	return elfHeader->e_entry;

}





