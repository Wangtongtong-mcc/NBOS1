#include "x86.h"
#include "kernel.h"

struct lock video_lock;
struct lock disk_lock;


/*
 * 初始化锁
 * */
void init_lock(){
	video_lock.value=0;
	disk_lock.value=0;
}

/*
 * 获取锁
 * */
int acquire(struct lock* lock) {
	//获取lock值
	while (xchg(&(lock->value), 1) != 0)
		;
}

/*
 * 释放锁
 * */
int release(struct lock* lock) {
	lock->value=0;
}

