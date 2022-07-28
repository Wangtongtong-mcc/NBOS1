#include "unistd.h"

int write(int fd, void * buf, unsigned int size)  {

	int result;
	asm("int $0x80"\
	:"=a"(result)\
	:"a"(SYS_write),\
	"b"(fd),"c"(buf),"d"(size));
	return result;

}

void clear_my_screen(void){

	asm("int $0x80"\
	::"a"(SYS_clear_screen));

}

int open(const char *path, int oflag){

	int result;
	asm("int $0x80"\
	:"=a"(result)\
	:"a"(SYS_open),\
	"b"(path),"c"(oflag));
	return result;

}

int read(int fd, void *buf, unsigned int size){

	int result;
	asm("int $0x80"\
	:"=a"(result)\
	:"a"(SYS_read),\
	"b"(fd),"c"(buf),"d"(size));
	return result;

}

int close (int fd){

	int result;
	asm("int $0x80"\
	:"=a"(result)\
	:"a"(SYS_close),\
	"b"(fd));
	return result;

}

int creat(const char *path, unsigned short mode){

	int result;
	asm("int $0x80"\
	:"=a"(result)\
	:"a"(SYS_creat),\
	"b"(path),"c"(mode));
	return result;

}

int fork(void){

	int result;
	asm("int $0x80"\
	:"=a"(result)\
	:"a"(SYS_fork));
	return result;
}

int execve(const char *pathname, char * const argv[], char * const envp[]){

	int result;
	asm("int $0x80"\
	:"=a"(result)\
	:"a"(SYS_execve),\
	"b"(pathname),"c"(argv),"d"(envp));
	return result;

}

int wait(int *statloc){

	int result;
	asm("int $0x80"\
	:"=a"(result)\
	:"a"(SYS_wait),\
	"b"(statloc));
	return result;

}


void _exit(int status){

	asm("int $0x80"\
	::"a"(SYS_exit),\
	"b"(status));

}

