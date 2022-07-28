#include "kernel.h"
#include "print.h"
#include "memory.h"
#include "file.h"
#include "process.h"
#include "keyboard.h"

extern struct cpu mycpu;
extern unsigned int pagedir[PAGE_STRUCT_ENTRIES];
extern struct pcb pcblist[PCB_MAX];
//struct page_dir_item_tt tt;

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
	// 若为标准输入，调用相应的函数
	if (fd == STDIN_FILENO){
		int size = readStdin((char  *)buf);
		return size;
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



/**
 * 用于将当前进程的内存资源复制到目标进程中
 * 1. 复制页目录
 * 2. 遍历目标进程的页目录项，针对有效的页目录项，复制该页目录项对应的页表，并修改该页目录项
 * 3. 遍历目标进程的页表项，针对有效的页表项，复制该页表项对应的页，并修改该页表项
 *
 * @param p
 *
 */
void copy_process_mem(struct pcb * p){

	// 1. 复制页目录
	// 1.1 先申请一个内核物理页，作为目标进程的页目录,并填充0
	struct page_dir_t* new_process_page_dir_ptr = (struct page_dir_t*) alloc_page();
	fill((unsigned char *)new_process_page_dir_ptr,PAGESIZE,0);
	p->pagedir = (unsigned char*)new_process_page_dir_ptr;

	// 1.2 将当前进程的页目录复制到目标进程页目录
	memcopy((unsigned char *)mycpu.current_process->pagedir,(unsigned char *)new_process_page_dir_ptr, PAGESIZE);

	// 2. 遍历目标进程的页目录项，针对有效的页目录项，复制该页目录项对应的页表，并修改该页目录项
	// 2.1 获取目标进程页目录的首个页目录项的指针
	struct page_dir_item_t * base_page_dir_item_ptr = (struct page_dir_item_t *)new_process_page_dir_ptr;
	// 2.2 遍历用户空间的每一个页目录项
	for(int i = 0; i < 0x300; i++){
		// 2.2.1 获取当前页目录项指针
		struct page_dir_item_t * current_process_page_dir_item_ptr = base_page_dir_item_ptr + i;
		// 2.2.2 判断该页目录项是否有效
		if(current_process_page_dir_item_ptr->present == PAGE_PRESENT_TRUE){   // 该目录项有效

			// 2.2.2.1 新申请页表,并填充0
			struct page_table_t * new_process_page_table_ptr = (struct page_table_t *) alloc_page();
			fill((unsigned char *)new_process_page_table_ptr,PAGESIZE,0);

			// 2.2.2.2 将当前进程的页表复制到新页表
			struct page_table_t * old_process_page_table_ptr = (struct page_table_t *)PHY_2_VIR((current_process_page_dir_item_ptr->address) << 12);
			memcopy((unsigned char *)old_process_page_table_ptr, (unsigned char *)new_process_page_table_ptr, PAGESIZE);

			// 2.2.2.3 修改当前页目录项
			generate_page_dir_item(current_process_page_dir_item_ptr,(struct page_table_t *)(VIR_2_PHY((unsigned int)new_process_page_table_ptr)),PAGE_PRIVILEGE_USER);

			// 2.2.2.4 获取新页表的第一个页表项的地址
			struct page_table_item_t * base_page_table_item_ptr = (struct page_table_item_t *)new_process_page_table_ptr;

			// 2.2.2.5 遍历新页表的页表项
			for(int j = 0; j < 1024; j++){
				// 2.2.2.5.1 获取当前页表项的地址
				struct page_table_item_t * current_page_table_item_ptr = base_page_table_item_ptr + j;

				// 2.2.2.5.2 判断当前页表项是否有效
				if(current_page_table_item_ptr->present == PAGE_PRESENT_TRUE){      //该页表项有效

					// 2.2.2.5.2.1 新申请页
					struct page_t * new_process_page = (struct page_t*)alloc_page();

					// 2.2.2.5.2.2 将当前进程的页复制到新申请的页
					struct page_t * old_process_page = (struct page_t *)PHY_2_VIR((current_page_table_item_ptr->address)<<12);
					memcopy((unsigned char*) old_process_page,(unsigned char*) new_process_page,PAGESIZE);

					// 2.2.2.5.2.3 修改当前页表项，指向新申请的页
					generate_page_table_item(current_page_table_item_ptr,(struct page_t *)(VIR_2_PHY((unsigned int)new_process_page)),PAGE_PRIVILEGE_USER);
				}
			}

		}
	}

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
	for (int i = 0; i < FILE_MAX_InPROCESS; i++){
		p->fdlist[i].open_flag = mycpu.current_process->fdlist[i].open_flag;
		p->fdlist[i].ofptr = mycpu.current_process->fdlist[i].ofptr;
	}
	// 2. 复制进程内存资源
	copy_process_mem(p);

	// 3. 复制上下文
	copyContext(ptr,p);


	// 4. 返回pid
	return p->pid;


}

/**
 * 释放当前进程的内存资源
 *
 *
 */
void free_process_user_mem(void){

	// 1. 获取当前进程页目录的第一个页目录项的地址
	struct page_dir_item_t * base_page_dir_item_ptr = (struct page_dir_item_t *)mycpu.current_process->pagedir;

	// 2. 遍历页目录中用户空间的页目录项
	for (int i = 0; i < 0x300; i++){

		// 2.1 获取当前的页目录项
		struct page_dir_item_t * current_page_dir_item_ptr = base_page_dir_item_ptr + i;

		// 2.2 判断当前页目录项是否有效
		if(current_page_dir_item_ptr->present == PAGE_PRESENT_TRUE){ //有效

			// 2.2.1 获取当前页目录项对应页表的地址
			struct page_table_t * page_table_ptr = (struct page_table_t *)PHY_2_VIR((current_page_dir_item_ptr->address) << 12);

			// 2.2.2 获取当前页表的第一个页表项的地址
			struct page_table_item_t * base_page_table_item_ptr = (struct page_table_item_t *)page_table_ptr;

			// 2.2.3 遍历页表中的页表项
			for (int j = 0; j < 1024; j++){

				// 2.2.3.1 获取当前页表项的地址
				struct page_table_item_t * current_page_table_item_ptr = base_page_table_item_ptr + j;

				// 2.2.3.1 判断该页表项是否有效
				if (current_page_table_item_ptr->present == PAGE_PRESENT_TRUE){ // 有效

					// 2.2.3.1.1 获取当前页表项对应的页的地址
					struct page_t * page_ptr = (struct page_t *)PHY_2_VIR((current_page_table_item_ptr->address) << 12);
					//  2.2.3.1.2 释放该页
					free_page(page_ptr);
				}
			}

			// 2.2.4 释放本页表
			free_page((struct page_t *)page_table_ptr);
		}
	}

	// 3. 释放页目录
	//free_page((struct page_t *)mycpu.current_process->pagedir);

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


	//  2. 释放进程用户空间的内存资源
	free_process_user_mem();

	// 3.加载进程，替换当前用户进程
	void * entry = loadProcess(&in);
	if ( entry == NULL){
		return -1;
	};

	// 4. 修改进程寄存器
	// 申请一个页作为
	struct page_t * stack_page_phy_addr = (struct page_t *)VIR_2_PHY((unsigned int)alloc_page());
	page_map((struct page_t *)(USER_STACK_START & 0xfffff000),stack_page_phy_addr,PAGE_PRIVILEGE_USER);


	ptr->esp = USER_STACK_START;
	ptr->eip = (unsigned int )entry;

}

/* sys_wait：在一个子进程终止前，使其调用者阻塞
 * statloc指向子进程的终止状态
 * 成功返回子进程id，出错返回0或-1
 *
 * */
int sys_wait(struct intr_context *ptr){

	// 0. 获取参数
	int * statloc = (int *)ptr->ebx;

	// 1. 取当前进程的进程id
	unsigned int pid = mycpu.current_process->pid;

	// 2. 看当前进程是否有子进程，没有子进程则出错返回
	if(!haveChildren(pid)){
		return -1;
	}

	// 3. 遍历进程链表，看是否有当前的子进程已经终止
	// 没有终止的子进程则阻塞，只要有一个终止的子进程则返回其pid
	for(;;){

		for(int i = 0; i < PCB_MAX; i++){
			// 先找到当前进程的子进程
			if(pcblist[i].father_pid != pid)
				continue;

			// 确认当前子进程是否终止，是的话，清空进程，返回其pid
			if(pcblist[i].pstate == ZOMBIE){
				// 取pid
				unsigned int child_pid = pcblist[i].pid;

				// 清空本进程控制块
				pcblist[i].pagedir = 0;
				pcblist[i].pstate = FREE;
				pcblist[i].father_pid = 0;
				pcblist[i].pid = 0;
				pcblist[i].next_vir_addr = 0;

				// 返回其进程id
				return child_pid;
			}

		}

	}

}


/* sys_exit用于进程终止时的资源处理
 *
 * */
int sys_exit(struct intr_context *ptr){

	// 0. 获取参数
	int status = ptr->ebx;

	// 1. 取当前进程的进程id，并获取当前进程控制块
	unsigned int pid = mycpu.current_process->pid;
	struct pcb * p = getPCB(pid);

	// 2. 释放本进程占用的内存资源
	// 即用户空间的一个页，内核空间的页目录和页表两个页
	free_page((struct page_t*)p->pagedir);

	// 3. 关闭本进程打开的文件
	for(int i = 0; i < FILE_MAX_InPROCESS; i++){
		if(p->fdlist[i].open_flag == OPEN){

			// 清空文件表中的对应项
			struct open_file * ofptr = p->fdlist[i].ofptr;
			delFile(ofptr);

			// 清空进程文件描述符表对应项
			p->fdlist[i].ofptr = NULL;
			p->fdlist[i].open_flag = CLOSE;
		}
	}

	// 4. 设置进程的状态
	p->pstate = ZOMBIE;

	// 5. 切换到其他RUNNABLE的程序
	struct pcb * next;
	for(int i = 0; i < PCB_MAX; i++){
		if (pcblist[i].pstate == RUNNABLE){
			next = &(pcblist[i]);
			break;
		}
	}
	// 设置cpu当前进程
	mycpu.current_process = next;

	// 切换页目录(PCD=PWT=0)
	unsigned int dir = VIR_2_PHY((unsigned int )next->pagedir);
	asm("mov %%eax, %%cr3"::"a"(dir));

	// 切换上下文
	switchCtx(&(next->ctx));

}