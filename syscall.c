#include "kernel.h"
#include "print.h"
#include "memory.h"
#include "file.h"
#include "process.h"

extern struct cpu mycpu;
extern unsigned int pagedir[PAGE_STRUCT_ENTRIES];

/* sys_clear为清理屏幕函数
 * */
int sys_clear(struct intr_context *ptr){
	clear_screen();
	return 0;
}

/* sys_write为向文件中写入的函数
 * fd为文件描述符，ptr为指向待写入内容的指针，size待写入内容大小
 * 返回已写入的字节大小
 * */
int sys_write(struct intr_context *ptr){

	// 0. 从上下文中取参数
	int fd = (int) ptr->ebx;
	void * buf = (void *) ptr->ecx;
	unsigned int size = ptr->edx;

	// 1. 参数有效性判断
	if (fd >= FILE_MAX_InPROCESS || fd < 0 || buf == NULL || size < 0){
		return -1;
	}

	// 2. 判断fd是否已打开
	// 取当前进程
	struct pcb * p = mycpu.current_process;
	if (!isOpen(fd,p)){
		return -1;
	}

	// 3. 判断是否有写权限
	// 取当前fd指向的文件表项
	struct fd current_fd= p->fdlist[fd];
	struct open_file * file = current_fd.ofptr;
	// 既非只写又非读写，则返回错误
	if(!(haveAccess(file,O_WRONLY) || haveAccess(file, O_RDWR))){
		return -1;
	}

	// 4. 执行写操作,根据不同文件类型调用不同写函数
	// 若为标准输出或者标准错误，直接调用print即可
	if (fd == STDOUT_FILENO || fd == STDERR_FILENO){
		print((unsigned char * )buf);
		return size;
	}
	// 5.如果是其他文件, 读入文件对应的inode, 并执行对应的写操作
	if(file->f_inode_block_no <= 0 || file->f_inode_block_no > INODE_BLOCK_SIZE){
		return -1;
	}
	struct inode in;
	read_inode(file->f_inode_block_no,file->f_inode_offset,&in);

	if(in.mode == REGULAR_FILE){
		writeFile(file,&in,buf,size);
	}

	// 6.更新inode并将其写回磁盘
	in.size +=size;
	writeInode(&in);

	return size;
}

/* sys_read为向文件中读取size大小的内容
 * fd为文件描述符，buf为指向待写入内容的指针，size待写入内容大小
 * 返回读取的字节大小
 * */
int sys_read(struct intr_context *ptr){

	// 0. 从上下文中取参数
	int fd = (int) ptr->ebx;
	void * buf = (void *) ptr->ecx;
	unsigned int size = ptr->edx;

	// 1. 参数有效性判断
	if (fd >= FILE_MAX_InPROCESS || fd < 0 || buf == NULL || size < 0){
		return -1;
	}

	// 2. 判断fd是否已打开
	// 取当前进程
	struct pcb * p = mycpu.current_process;
	if (!isOpen(fd,p)){
		return -1;
	}

	// 3. 判断是否有读权限
	// 取当前fd指向的文件表项
	struct fd current_fd= p->fdlist[fd];
	struct open_file * file = current_fd.ofptr;
	// 既非只读又非读写，则返回错误
	if(!(haveAccess(file,O_RDONLY) || haveAccess(file, O_RDWR))){
		return -1;
	}

	// 4. 执行读操作，根据不同文件类型调用不同读函数
	// 若为标准输入，
	if (fd == STDIN_FILENO){
		// 读取键盘输入函数

	}
	// 5. 若是其他文件，读入文件对应的inode，并调用对应函数进行读操作
	if(file->f_inode_block_no <= 0 || file->f_inode_block_no > INODE_BLOCK_SIZE){
		return -1;
	}
	struct inode in;
	read_inode(file->f_inode_block_no,file->f_inode_offset,&in);

	int readSize;
	if (in.mode == REGULAR_FILE){
		readSize = readFile(file,&in,buf,size);
	}

	// 6. 更新inode中的时间等属性，并将其写回磁盘

	writeInode(&in);

	return readSize;

}

/* sys_open用于打开文件
 * path为文件路径，oflag为打开文件属性
 * */
int sys_open(struct intr_context *ptr){

	// 0. 从上下文中取参数
	char *path = (char *) ptr->ebx;
	int oflag= (int) ptr->ecx;

	struct inode in;
	// 1. 将该文件对应的inode加载到内存中
	if(seekFileInode(path, &in) == -1){
		return -1;
	}

	// 2. 寻找进程可用的最小fd
	// 取当前进程
	struct pcb * p = mycpu.current_process;
	int fd = getMinfd(p);

	// 3. 填充内核的文件表
	struct open_file * ofptr = NULL;
	if(fd > 0){
		ofptr = setOpenFileList(oflag,&in);
	}

	// 4. 填充进程控制块的文件描述符表
	if (ofptr != NULL){
		p->fdlist[fd].ofptr = ofptr;
		p->fdlist[fd].open_flag = OPEN;
	}
	// 5. 返回fd
	return fd;

}

/* sys_close用于关闭文件
 * fd为文件描述符
 * */
int sys_close(struct intr_context *ptr){

	// 0. 从上下文中取参数
	int fd= (int) ptr->ebx;

	// 1. 检查参数有效性
	// 取当前进程
	if (fd >= FILE_MAX_InPROCESS || fd < 0) {
		return -1;
	}

	// 2. 检查fd是否已经打开
	// 取当前进程
	struct pcb * p = mycpu.current_process;
	if(!isOpen(fd,p)){
		return -1;
	}

	// 3. 清空文件表中的对应项
	struct open_file * ofptr = p->fdlist[fd].ofptr;
	delFile(ofptr);

	// 4. 清空进程文件描述符表对应项
	p->fdlist[fd].ofptr = NULL;
	p->fdlist[fd].open_flag = CLOSE;

	return 0;

}

/* sys_creat用于创建新文件
 * path为文件路径，mode为文件权限
 * 返回新文件的描述符
 * */
int sys_creat(struct intr_context *ptr){

	// 0. 从上下文中取参数
	unsigned char *path = (unsigned char *) ptr->ebx;
	unsigned short mode = (unsigned short) ptr->ecx;

	struct inode in, base_inode;
	// 1. 先为新文件分配一个新的inode结构in
	if (allocInode(&in) == -1){
		return -1;
	}

	// 2. 为新文件分配新的datablock，写入inode
	if (allocBlock(&in) == -1){
		return -1;
	}

	// 3. 将新文件的mode填到其inode中
	in.mode = mode;

	// 4. 寻找该文件的上级目录base_inode
	if (seekBaseDirectory(path,&base_inode) == -1){
		return -1;
	}


	// 5. 在上级目录填写新文件的名称及inode指针
	if(writeDirectory(&base_inode,path,&in) == -1){
		return -1;
	}

	// 6. 将新inode写回磁盘
	if(writeInode(&in) == -1){
		return -1;
	}

	// 7. 打开该新文件(以读写方式),返回fd
	struct intr_context open_context;
	open_context.ebx = (unsigned int)path;
	open_context.ecx = (unsigned int)O_RDWR;

	return sys_open(&open_context);

}

/* sys_fork用于新创建一个进程，为当前进程的子进程
 * 当前进程返回子进程的pid
 * 子进程返回0
 * */
int sys_fork(struct intr_context *ptr){

	// 0. 新建进程（即寻找空闲进程控制块）
	struct pcb * p = allocProcess();
	if(p == NULL){
		return -1;
	}

	// 1. 复制进程属性
	p->size = mycpu.current_process->size;
	p->next_vir_addr = mycpu.current_process->next_vir_addr;
	for (int i = 0; i < FILE_MAX_InPROCESS; i++){
		p->fdlist[i].open_flag = mycpu.current_process->fdlist[i].open_flag;
		p->fdlist[i].ofptr = mycpu.current_process->fdlist[i].ofptr;
	}
	p->pagedir = mycpu.current_process->pagedir;

	// 2. 复制上下文
	copyContext(ptr,p);
	// 3. 返回pid
	return p->pid;


}

/* sys_execve用于执行一个进程，为execv的系统调用
 * 主要使用path指向的磁盘可执行文件替换调用程序的正文，数据等
 * 成功不返回，出错返回-1
 *
 * */
int sys_execve(struct intr_context *ptr){

	// 0. 获取参数
	char * path = (char *)ptr->ebx;
	char ** argv = (char **)ptr->ecx;
	char ** envp = (char **)ptr->edx;

	// 1. 判断参数有效性
	// 首先，如果调用程序是内核，则返回，因为内核必须一直存在于内存，不可替换
	struct pcb * p = mycpu.current_process;
	if (p->pid == 0){
		return -1;
	}
	// 其次，该文件必须是普通文件
	struct inode in;
	seekFileInode(path,&in);
	if (in.mode != REGULAR_FILE){
		return -1;
	}

	// 2.先将进程加载到内核空间的缓存处，获取进程入口地址
	int entry = loadProcess(&in);
	if ( entry == -1){
		return -1;
	};

	// 3. 为该进程新建页目录,并复制内核空间的页目录
	p->pagedir = kalloc();
	memcopy((unsigned char *)(&(pagedir[0x300])),(unsigned char *)(p->pagedir + 0x300),PAGESIZE-0xc00);

	// 4. 修改进程状态
	p->ctx.esp = PAGESIZE;
	p->ctx.eip = entry;
	p->pstate = RUNNABLE;

	// 5. 切换页目录(PCD=PWT=0)
	unsigned int dir = VIR_2_PHY((unsigned int )p->pagedir);
	asm("mov %%eax, %%cr3"::"a"(dir));

	// 6. 将新进程复制到它自己的地址空间（虚拟地址从0x00000000开始，物理分配UFREE_START之上的页）
	unsigned int * user = ualloc(&(p->next_vir_addr),p->pagedir);
	memmov((unsigned char *)BUFFER_START, (unsigned char *)user,(unsigned int)PAGESIZE);

	// 7. 切换进程
	struct pcb *father = mycpu.current_process;
	father->pstate = RUNNABLE;

	mycpu.current_process = p;
	p->pstate = RUNNING;

	switchktou(&(father->ctx),&(p->ctx));

}