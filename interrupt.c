#include "interrupt.h"
#include "print.h"
#include "x86.h"
#include "syscall.h"

struct gate_desc idt[IDT_ENTRIES];              // idt中包含256个描述符
char * intr_name[IDT_ENTRIES];                  // 存放中断名称
struct idtr_structure idtr;
// 系统调用函数指针数组
sn_ptr sys_call_table[IDT_ENTRIES] = {[2]=sys_fork, sys_read, sys_write, sys_open, sys_close, [8]=sys_creat,[11]=sys_execve, [72]=sys_clear};

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
	idt[0] = make_idt_desc(divide_error_handler,TRAP_GATE_ATTRIBUTE);
	intr_name[0] = "#DE Divide Error";
	// #DB
	idt[1] = make_idt_desc(debug_handler,TRAP_GATE_ATTRIBUTE);
	intr_name[1] = "#DB Debug";
	// NMI
	idt[2] = make_idt_desc(nmi_handler,TRAP_GATE_ATTRIBUTE);
	intr_name[2] = "NMI Interrupt";
	// #BP
	idt[3] = make_idt_desc(breakpoint_handler, SYSCALL_INTERRUPT_GATE_ATTRIBUTE);
	intr_name[3] = "#BP Breakpoint";
	// #OF
	idt[4] = make_idt_desc(overflow_handler, SYSCALL_INTERRUPT_GATE_ATTRIBUTE);
	intr_name[4] = "#OF Overflow";
	// #BR
	idt[5] = make_idt_desc(bound_handler,SYSCALL_INTERRUPT_GATE_ATTRIBUTE);
	intr_name[5] = "#BR Bound Range Exceeded";
	// #UD
	idt[6] = make_idt_desc(invalid_opcode_handler,TRAP_GATE_ATTRIBUTE);
	intr_name[6] = "#UD Invalid Opcode(Undefined Opcode)";
	// #DF
	idt[8] = make_idt_desc(double_fault_handler,INTERRUPT_GATE_ATTRIBUTE);
	intr_name[8] = "#DF Double Fault";
	// #TS
	idt[10] = make_idt_desc(invalid_tss_handler,TRAP_GATE_ATTRIBUTE);
	intr_name[10] = "#TS Invalid TSS";
	// #NP
	idt[11] = make_idt_desc(segment_not_present_handler,TRAP_GATE_ATTRIBUTE);
	intr_name[11] = "#NP Segment Not Present";
	// #SS
	idt[12] = make_idt_desc(stack_segment_handler,TRAP_GATE_ATTRIBUTE);
	intr_name[12] = "#SS Stack Segment Fault";
	// #GP
	idt[13] = make_idt_desc(general_protection_handler, TRAP_GATE_ATTRIBUTE);
	intr_name[13] = "#GP General Protection";
	// #PF
	idt[14] = make_idt_desc(page_fault_handler,TRAP_GATE_ATTRIBUTE);
	intr_name[14] = "#PF Page Fault";
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
	} else{
		// 其余的中断统一处理，打印中断信息
		general_intr_handler(ptr);
	}


}






