#include "interrupt.h"
#include "print.h"
#include "x86.h"
#include "syscall.h"
#include "process.h"

struct gate_desc idt[IDT_ENTRIES];              // idt中包含256个描述符
char * intr_name[IDT_ENTRIES];                  // 存放中断名称
struct idtr_structure idtr;
extern struct pcb pcblist[PCB_MAX];             // pcb链表
extern struct cpu mycpu;

// 系统调用函数指针数组
sn_ptr sys_call_table[IDT_ENTRIES] = {[2]=sys_fork, sys_read, sys_write, sys_open, sys_close, [8]=sys_creat,[11]=sys_execve, [72]=sys_clear};

void init_clock(){

	/* 8253有三个计数器(16位)，其中counter0（port: 0x40）用于连接8259a的IRQO
	 * 默认情况下，计数器中的数值位最大值65535，输入频率位1193180，即每秒中断1193180/65535次
	 * 初始化8253，需要设置8253模式控制器（port:0x43）
	 *    7   6   5   4    3   2    1    0
	 *  ------------------------------------
	 *	|选计数器| 读写锁位 |   模式位    | 0 |
	 *	------------------------------------
	 * */

	// 设置8253模式控制器，选0号计数器，模式2，使用二进制
	outb(MODE_CONTROL,0x34);

	// 设置计数器频率
	// 读写锁位为11时，先读写低字节，再读写高字节
	outb(COUNTER0,(char)(COUNT));
	outb(COUNTER0,(char)(COUNT >> 8));
}



/* init_pic用于初始化8259A芯片
 * */
void init_pic(void){

	print("Initial PIC!\n");

	// 初始化主片，必须按顺序设置ICW1到ICW4
	// ICW1：边沿触发，级联，需要写ICW4
	outb(PIC_M1,0x11);
	// ICW2: 起始中断向量号为0x20(0-19为处理器内部异常，20-31为Intel保留)
	outb(PIC_M2,0x20);
	// ICW3: 主片中IRQ2用于级联从片
	outb(PIC_M2,0x04);
	// ICW4: 8086处理器，非自动结束中断
	outb(PIC_M2,0x01);

	// 初始化从片，必须按顺序设置ICW1到ICW4
	// ICW1：边沿触发，级联，需要写ICW4
	outb(PIC_S1,0x11);
	// ICW2: 起始中断向量号为0x28
	outb(PIC_S2,0x28);
	// ICW3: 主片中IRQ2用于级联从片
	outb(PIC_S2,0x02);
	// ICW4: 8086处理器，非自动结束中断
	outb(PIC_S2,0x01);

	// 初始化时钟芯片8253
	init_clock();

	// OCW1: 打开时钟中断IRQ0
	outb (PIC_M2, 0xfe);
	outb(PIC_S2,0xff);

	print("Init_pic done!\n");

}

/* init_interrupt用于初始化中断向量表
 * */
void init_interrupt(void){
	print("Initial interrupt!\n");

	// 所有中断的初始默认处理程序为通用的中断处理程序，循环填充idt各个条目
	struct gate_desc idt_desc = make_idt_desc(other_exception_handler,INTERRUPT_GATE_ATTRIBUTE);
	for (int i=0 ; i<256; i++){
		idt[i] = idt_desc;
		intr_name[i] = "unknown";
	}
	// #DE异常
	idt[0] = make_idt_desc(divide_error_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[0] = "#DE Divide Error";
	// #DB
	idt[1] = make_idt_desc(debug_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[1] = "#DB Debug";
	// NMI
	idt[2] = make_idt_desc(nmi_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[2] = "NMI Interrupt";
	// #BP
	idt[3] = make_idt_desc(breakpoint_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[3] = "#BP Breakpoint";
	// #OF
	idt[4] = make_idt_desc(overflow_handler, INTERRUPT_GATE_ATTRIBUTE);
	intr_name[4] = "#OF Overflow";
	// #BR
	idt[5] = make_idt_desc(bound_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[5] = "#BR Bound Range Exceeded";
	// #UD
	idt[6] = make_idt_desc(invalid_opcode_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[6] = "#UD Invalid Opcode(Undefined Opcode)";
	// #DF
	idt[8] = make_idt_desc(double_fault_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[8] = "#DF Double Fault";
	// #TS
	idt[10] = make_idt_desc(invalid_tss_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[10] = "#TS Invalid TSS";
	// #NP
	idt[11] = make_idt_desc(segment_not_present_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[11] = "#NP Segment Not Present";
	// #SS
	idt[12] = make_idt_desc(stack_segment_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[12] = "#SS Stack Segment Fault";
	// #GP
	idt[13] = make_idt_desc(general_protection_handler, INTERRUPT_GATE_ATTRIBUTE);
	intr_name[13] = "#GP General Protection";
	// #PF
	idt[14] = make_idt_desc(page_fault_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[14] = "#PF Page Fault";
	// 时钟中断
	idt[32] = make_idt_desc(timer_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[32] = "#Timer INTERRUPT";
	// 系统调用
	idt[128] = make_idt_desc(syscall_handler,SYSCALL_INTERRUPT_GATE_ATTRIBUTE);
	intr_name[128] = "SYSTEM CALL";
	// idt的界限和地址
	idtr.idt_size = 256*8-1;
	idtr.idt_addr_low = (unsigned short)((unsigned int) idt & 0x0000ffff);
	idtr.idt_addr_high = (unsigned short)(((unsigned int)idt & 0xffff0000)>>16);

	// 加载idtr
	asm("lidt idtr");

	print("Init_interrupt done!\n");

}


/* make_idt_desc用于组合一个idt描述符，可以是任务门，陷阱门，中断门
 * */
struct gate_desc make_idt_desc (void(* function)(void),short attribute){
	struct gate_desc idt_desc;
	idt_desc.attribute = attribute;
	idt_desc.selector = KCODE_SELECTOR;
	idt_desc.offset_high = (short)(((unsigned int)function & 0xffff0000)>>16);
	idt_desc.offset_low = (short)((unsigned int)function & 0x0000ffff);

	return idt_desc;
}

/* renew_handler用于以新的函数更新中断处理程序
 * */
void renew_handler(unsigned int vector, void(* function)(void)){
	if(vector == 0x80){
		idt[vector] = make_idt_desc(function, SYSCALL_INTERRUPT_GATE_ATTRIBUTE);
	}
	idt[vector] = make_idt_desc(function, INTERRUPT_GATE_ATTRIBUTE);

}



/* page_np_handler用于处理缺页异常
 *
 * */
void page_np_handler(unsigned int cr2, unsigned int errcode){
	print("Page is not present!\n");
}

/* general_intr_handler为除系统调用外，其他中断的处理函数
 * 主要用于打印中断信息并停机
 */
void general_intr_handler(struct intr_context * ptr){

	print("Exception Occur!\n");

	// 分别打印中断向量，异常名称，错误码
	print("Vector NO.:");
	printn(ptr->inter_no);

	print("\nException name:");
	print(intr_name[ptr->inter_no]);

	print("\nError Code:");
	printn(ptr->error_code);

	// 打印发生中断时的CS，EIP，EFLAGS
	print("\nCS\tEIP\tEFLAGS\n");
	printn(ptr->cs);
	print("\t");
	printn(ptr->eip);
	print("\t");
	printn(ptr->eflags);

	// 停机
	asm("hlt");
}

/* do_timer_handler为时钟中断处理函数
 */
void do_timer_handler(struct intr_context * ptr){

	// 找出pcb链表中可运行的进程
	struct pcb * p;
	for(int i = 0; i < PCB_MAX; i++){
		if (pcblist[i].pstate == RUNNABLE){
			p = &(pcblist[i]);
			break;
		}
	}

	// 设置cpu当前进程
	struct pcb * father = mycpu.current_process;
	mycpu.current_process = p;

	// 切换页目录(PCD=PWT=0)
	unsigned int dir = VIR_2_PHY((unsigned int )p->pagedir);
	asm("mov %%eax, %%cr3"::"a"(dir));

	// 切换进程状态
	father->pstate = RUNNABLE;
	p->pstate = RUNNING;

	// 切换上下文
	switchktou(&(father->ctx),&(p->ctx));

	// print("In timer!");

}

/* interrupt_assign为中断分派函数，由中断入口函数调用
 * 根据中断向量的不同，执行不同的中断处理函数
 */
void interrupt_assign(struct intr_context * ptr){
	unsigned int intr_no = ptr->inter_no;
	// 单独处理系统调用
	if (intr_no == 0x80){
		// 取功能号
		unsigned int fun_no = ptr->eax;
		// 调用具体的系统调用函数
		sys_call_table[fun_no](ptr);
	} else if(intr_no == 0x20){
		// 调用时钟中断处理函数
		do_timer_handler(ptr);
	} else{
		// 其余的中断统一处理，打印中断信息
		general_intr_handler(ptr);
	}


}






