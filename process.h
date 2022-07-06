#ifndef _KERNEL_PROCESS_H
#define _KERNEL_PROCESS_H

#include "kernel.h"


void init_pcb(void);
void first_user_process(void);
struct pcb * create_process(void);
void start_user_process(void);
void switchktou(struct context * source, struct context * dest);

struct pcb * allocProcess(void);
int copyContext(struct intr_context *father,struct pcb *p);
int loadProcess(struct inode * in);
void switchCtx(struct context * ctx);



#endif