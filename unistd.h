#ifndef _USR_LIB_UNISTD_H
#define _USR_LIB_UNISTD_H

// 标准IO文件描述符
#define STDIN_FILENO 0 /* standard input file descriptor */
#define STDOUT_FILENO 1 /* standard output file descriptor */
#define STDERR_FILENO 2 /* standard error file descriptor */

/* open/fcntl - NOCTTY, NDELAY isn't implemented yet */
#define O_ACCMODE	00003
#define O_RDONLY	   00
#define O_WRONLY	   01
#define O_RDWR		   02
#define O_CREAT		00100	/* not fcntl */
#define O_EXCL		00200	/* not fcntl */
#define O_NOCTTY	00400	/* not fcntl */
#define O_TRUNC		01000	/* not fcntl */
#define O_APPEND	02000
#define O_NONBLOCK	04000	/* not fcntl */
#define O_NDELAY	O_NONBLOCK

//  文件类型
#define REGULAR_FILE 0
#define DIRECTORY_FILE 1


// 系统调用功能号
#define SYS_exit 1
#define SYS_fork 2
#define SYS_read 3
#define SYS_write 4
#define SYS_open 5
#define SYS_close 6
#define SYS_wait 7
#define SYS_creat 8
#define SYS_execve 11
#define SYS_clear_screen 72



void _exit(int status);
// 子进程返回0，父进程返回子进程pid
int fork(void);
// 成功返回读到的字节数，出错返回-1
int read(int fd, void *buf, unsigned int size);
// 成功返回已写入的字节数，出错返回-1
int write(int fd, void *buf, unsigned int size);
// 成功返回文件描述符，出错返回-1
int open(const char *path, int oflag);
// 成功返回0，出错返回-1
int close (int fd);
// 成功，返回pid;出错，返回-1；
int wait(int *statloc);
// 成功返回文件描述符，出错返回-1
int creat(const char *path, unsigned short mode);
// 成功，不返回；出错，返回-1
int execve(const char *pathname, char * const argv[], char * const envp[]);
void clear_my_screen(void);



// 成功返回新偏移量，出错返回-1
// int lseek(int fd, long offset, int whence);
// 成功返回新文件描述符，出错返回-1
//int dup(int fd);
//int dup2(int fd, int fd2);
// 成功返回0，出错返回-1
//int fsync(int fd);
//int fdatasync(int fd);
//void sync(void);
// 出错返回-1，成功返回其他值
//int ioctl(int fd, int request);
//int openat(int fd, char *path, int oflag);
// 成功返回值依赖于cmd，出错返回-1
//int fcntl(int fd, int cmd);










#endif
