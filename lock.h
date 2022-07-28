#ifndef _KERNEL_LOCK_H
#define _KERNEL_LOCK_H


void init_lock(void);
int acquire(struct lock* lock);
int release(struct lock* lock);

#endif