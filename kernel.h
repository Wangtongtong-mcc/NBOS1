#ifndef _KERNEL_H
#define _KERNEL_H

#define NULL ((void*)0)
#define STACK_TOP 0xc0010000

// 进程
#define PCB_MAX 20                                              // 最大进程量
#define PID_MAX 60                                              // pid最大值，超过这个值后
enum run_state{FREE,BORN,RUNNABLE,RUNNING,ABORT};               // 进程状态
struct context{                                                 // 存储进程上下文
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int nesp;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;

	unsigned short gs;
	unsigned short padding6;
	unsigned short fs;
	unsigned short padding5;
	unsigned short es;
	unsigned short padding4;
	unsigned short ds;
	unsigned short padding3;

	unsigned int eip;
	unsigned short cs;
	unsigned short padding2;
	unsigned int eflags;
	unsigned int esp;
	unsigned short ss;
	unsigned short padding1;
};                                            // 进程上下文
#define OPEN 1                                                  // 文件是否打开
#define CLOSE 0
																// 用户进程所在扇区
#define SHELL_SECTOR 356
#define MYPRO_SECTOR 371

#define SHELL_BLOCKS 15
#define MYPRO_BLOCKS 5

struct task_struct{                                             // 任务状态信息
	unsigned int link;         // 上一个任务选择子
	unsigned int esp0;         // 内核栈指针
	unsigned short ss0;
	unsigned short padding1;
	unsigned int *esp1;
	unsigned short ss1;
	unsigned short padding2;
	unsigned int *esp2;
	unsigned short ss2;
	unsigned short padding3;
	void *cr3;                  // 内核页目录
	unsigned int *eip;
	unsigned int eflags;
	unsigned int eax;
	unsigned int ecx;
	unsigned int edx;
	unsigned int ebx;
	unsigned int *esp;
	unsigned int *ebp;
	unsigned int esi;
	unsigned int edi;
	unsigned short es;
	unsigned short padding4;
	unsigned short cs;
	unsigned short padding5;
	unsigned short ss;
	unsigned short padding6;
	unsigned short ds;
	unsigned short padding7;
	unsigned short fs;
	unsigned short padding8;
	unsigned short gs;
	unsigned short padding9;
	unsigned short ldt;
	unsigned short padding10;
	unsigned short t;
	unsigned short iomb;
};                                        // 任务结构






// 文件系统
#define FILE_MAX_InPROCESS 20                                        // 进程打开文件的最大量
#define FILE_MAX_INSYS 100                                           // 系统同一时间打开的最大文件量
#define MAX_BLOCKS_InFILE 15                                          // 一个文件最多占用的block
#define FILE_NAME_MAX 20                                             // 文件名称最大字节数

#define STDIN_FILENO 0                                           // 标注io_fd
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define O_RDONLY	   00                                        // 文件打开属性
#define O_WRONLY	   01
#define O_RDWR		   02
#define O_CREAT		00100
#define O_EXCL		00200

#define REGULAR_FILE 0                                          // 文件类型
#define DIRECTORY_FILE 1

#define BLOCK_SIZE 512                                         // 每个Block的大小
#define SUPER_BLOCK_SECTOR 300                                 // superBlock所在扇区
#define SUPER_BLOCK_NO 0                                       // superBlock所在block号
#define ROOT_DIR "/"                                           // 根目录名称

#define BLOCK_FREE 0                                            // block使用状态
#define BLOCK_BUSY 1

#define DataBlockBitSize 5                                      // Block位图占用的block数量
#define INODE_BLOCK_SIZE 50                                     // 磁盘中inode所占block数量
#define TOTAL_BLOCKS 1000                                       // 磁盘总共的Block数量
#define FILE_SYS_BLOCKS 700                                     // 文件系统占用的bLock数量（即除去引导扇区，loader,操作系统代码所占用的扇区）

struct inode{                                                   // 一个文件的元数据
	unsigned long size;                                         // 文件大小
	unsigned long atime;                                        // 最近访问时间
	unsigned long mtime;                                        // 最近修改时间
	unsigned long ctime;                                        // 最近文件属性修改时间
	unsigned short mode;                                        // 文件类型
	unsigned int next_inode_block_no;
	unsigned int next_inode_offset;
	unsigned int my_inode_block_no;
	unsigned int my_inode_offset;
	unsigned int block_no[MAX_BLOCKS_InFILE];
};

struct open_file{                                               // 内核为所有已打开文件维护的文件表项结构
	int oflags;                                                 // 文件打开属性
	long offset;                                                // 文件当前偏移量（下次读写的起始位置）
	unsigned int f_inode_block_no;
	unsigned int f_inode_offset;                               // 指向文件对应inode
	int isopen;
};

struct fd{                                                      // 每个进程打开文件的文件描述符结构
	int open_flag;                                              // 是否打开
	struct open_file * ofptr;                                   // 指向文件表项的指针
};

struct super_block{                                             // super block 结构
	unsigned int free_inode_block_no;                           // 空闲inode指针
	unsigned int free_inode_offset;
	unsigned int rootdir_inode_block_no;                        // 根目录inode指针
	unsigned int rootdir_inode_offset;
	unsigned int dataBlockBit_no[DataBlockBitSize];             // 存放dataBlock位图的10个block number
};

struct dir_item{                                                // 目录项结构
	char name[FILE_NAME_MAX];
	unsigned int inode_block_no;
	unsigned int inode_offset;
};







// 中断
#define IDT_ENTRIES 256                          // 中断数量
#define INTERRUPT_GATE_ATTRIBUTE 0x8e00          // 中断门（P=1;DPL=0）
#define TRAP_GATE_ATTRIBUTE 0x8f00               // 陷阱门（P=1;DPL=0）
#define TASK_GATE_ATTRIBUTE 0x8500               // 任务门
#define SYSCALL_INTERRUPT_GATE_ATTRIBUTE 0xee00  // 系统调用的门属性

struct gate_desc {                                      // idt中的描述符，可以是中断门，陷阱门， 任务门
	unsigned short offset_low;
	unsigned short selector;
	unsigned short attribute;
	unsigned short offset_high;
};

struct idtr_structure {                                 // idtr结构体，用于加载idtr，前两字节为idt大小， 后四字节为idt的地址
	unsigned short idt_size;
	unsigned short idt_addr_low;
	unsigned short idt_addr_high;
};

struct intr_context{                                    // 中断上下文结构
	// 进入中断后，由中断入口函数压入栈中
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int esp_current;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;


	unsigned short gs;                                  // 数据段寄存器
	unsigned short padding6;
	unsigned short fs;
	unsigned short padding5;
	unsigned short es;
	unsigned short padding4;
	unsigned short ds;
	unsigned short padding3;
	unsigned int inter_no;

	// 引发中断时，由cpu压入栈中
	unsigned int error_code;
	unsigned int eip;
	unsigned short cs;
	unsigned short padding2;
	unsigned int eflags;
	unsigned int esp;
	unsigned short ss;
	unsigned short padding1;
};

// 系统调用功能号
#define SYS_fork 2
#define SYS_read 3
#define SYS_write 4
#define SYS_open 5
#define SYS_close 6
#define SYS_creat 8
#define SYS_execve 11
#define SYS_clear_screen 72

// 系统调用函数指针
typedef int(*sn_ptr)(struct intr_context *ptr);





// 内存与硬盘
#define SECTOR_SIZE 512                                             // 磁盘扇区大小
#define KERNEL_SECTOR 5                                             // 内核在磁盘中的起始扇区

#define PAGESIZE 4096                   // 页面大小
#define PAGE_P 1                        // 页结构属性
#define PAGE_NP 0
#define PAGE_R 0
#define PAGE_RW 2
#define PAGE_S 0
#define PAGE_U 4
#define PAGE_STRUCT_ENTRIES 1024        // 一个页面包含的条目数量

#define PHY_SIZE 0x20000000             // 物理内存大小
#define VIR_SIZE 0xffffffff             // 虚拟内存大小
#define KMEM 0                          // 内存类型，是内核空间还是用户空间
#define UMEM 1

																	// 内核在内存中相关信息
#define KERNEL_PHY_START 0x10000                                    // 内核在物理内存中的起始位置
#define KERNEL_VIR_BASE 0xc0000000                                  // 内核虚拟地址空间的起始
#define BUFFER_START 0xc0000000                                     // 内核加载到0x10000以上，内核栈为0x10000以下（一个页），0x0000以上作为缓冲区来用
#define DEVICE_START 0x1e000000                                     // 设备起始虚拟地址

#define ALIGN(x) (unsigned char *)(((unsigned int)x & 0xfffff000) + PAGESIZE)        // 以页为单位向上对齐
#define VIR_2_PHY(x) ((x)-KERNEL_VIR_BASE)                                           // 虚拟地址转物理地址
#define PHY_2_VIR(x) ((x)+KERNEL_VIR_BASE)

																	// 内存分段相关信息
#define KRPL 0                                                      // 选择子
#define URPL 3
#define KCODE_SELECTOR ((1 << 3) + KRPL)
#define KDATA_SELECTOR ((2 << 3) + KRPL)
#define UCODE_SELECTOR ((3 << 3) + URPL)
#define UDATA_SELECTOR ((4 << 3) + URPL)
#define TSS_SELECTOR ((5 << 3) + KRPL)
																	// 描述符
#define GDT_G_4k 1
#define GDT_D_32 1
#define GDT_L 0
#define GDT_AVL 0
#define GDT_P 1
#define GDT_DPL0 0
#define GDT_S_SYS 0
#define GDT_TYPE_TSS 9
#define TSS_ATTR_HIGH ((GDT_G_4k << 7) + (GDT_D_32 << 6) + (GDT_L << 5) +( GDT_AVL << 4) + 0x0)
#define TSS_ATTR_LOW ((GDT_P << 7) +(GDT_DPL0 << 6) + (GDT_S_SYS << 4) + GDT_TYPE_TSS)

struct global_descriptor{
	unsigned short limit;
	unsigned short base_low;
	unsigned char base_mid;
	unsigned char attr_low;
	unsigned char attr_high;                     // 四位属性，四位界限最高位
	unsigned char base_high;
};








// 进程相关
struct pcb{                 // 存储进程信息，即进程控制块
	struct context ctx;     // 进程上下文
	unsigned long size;     // 进程大小
	unsigned int sector;    // 进程所在扇区
	unsigned int pid;       // 进程id
	unsigned int father_pid;// 父进程id

	enum run_state pstate;  // 进程状态
	unsigned int * pagedir; // 进程页目录(虚拟地址)
	unsigned int next_vir_addr; // 该进程下一个可分配的虚拟地址
	struct fd fdlist[FILE_MAX_InPROCESS];
};

struct cpu{                                         // cpu当前运行情况
	struct task_struct * current_task;              // 当前任务
	struct pcb * current_process;                   // 当前进程
	struct context kernel_context;                  // 内核上下文？？？？？
};














// 任务
void init_tss(void);
// 例程
unsigned int load_process(unsigned char * destpos, unsigned int sector);                    // 从磁盘中加载程序
void read_seg(unsigned char * target_location, unsigned int size, unsigned int start_sector, unsigned int offset);
void k_readsector(unsigned char* target_location, unsigned int sector_number);
void k_writesector(unsigned char* target_location, unsigned int sector_number);



#endif