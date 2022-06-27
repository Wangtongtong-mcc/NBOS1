#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H


#include "print.h"

#define assert(cond) if(!(cond)){\
		print("Condition Error!\n");\
		print(__FILE__);\
		printn(__LINE__);\
		asm("hlt");\
	}


#endif