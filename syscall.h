#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include "kernel.h"



int sys_fork(struct intr_context *ptr);
int sys_read(struct intr_context *ptr);
int sys_write(struct intr_context *ptr);
int sys_open(struct intr_context *ptr);
int sys_close(struct intr_context *ptr);
int sys_creat(struct intr_context *ptr);
int sys_clear(struct intr_context *ptr);
int sys_execve(struct intr_context *ptr);


#endif