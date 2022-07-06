#ifndef _KERNEL_INTERRUPT_H
#define _KERNEL_INTERRUPT_H

#include "kernel.h"

// 中断入口函数，即在idt中的函数
void divide_error_handler(void);
void debug_handler(void);
void nmi_handler(void);
void breakpoint_handler(void);
void overflow_handler(void);
void bound_handler(void);
void invalid_opcode_handler(void);
void double_fault_handler(void);
void invalid_tss_handler(void);
void segment_not_present_handler(void);
void stack_segment_handler(void);
void general_protection_handler(void);
void page_fault_handler(void);
void syscall_handler(void);
void other_exception_handler(void);
void timer_handler(void);
// 中断分派函数
void interrupt_assign(struct intr_context * ptr);
// 中断初始化函数
void init_interrupt(void);
struct gate_desc make_idt_desc (void(* function)(void),short attribute);
// 8259A芯片初始化函数
void init_pic(void);
// 缺页处理函数
void page_np_handler(unsigned int cr2, unsigned int errcode);


#endif