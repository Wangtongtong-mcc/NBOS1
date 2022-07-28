#ifndef _KERNEL_PROCESS_H
#define _KERNEL_PROCESS_H

#include "kernel.h"


void init_pcb(void);
void switch_to_user(void);
void first_user_process(void);
struct pcb * create_process(void);
void start_user_process(void);
void switchktou(struct context * source, struct context * dest);

struct pcb * allocProcess(void);
int copyContext(struct intr_context *father,struct pcb *p);
void *loadProcess(struct inode * in);
void switchCtx(struct context * ctx);
int haveChildren(unsigned int pid);
void schedule(void);
struct pcb * getPCB(unsigned int pid);



#endif