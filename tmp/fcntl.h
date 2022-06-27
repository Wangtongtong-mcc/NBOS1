#ifndef _USR_LIB_FCNTL_H
#define _USR_LIB_FCNTL_H

// 成功返回文件描述符，出错返回-1
int open(const char *path, int oflag);
int openat(int fd, const *path, int oflag);

// 成功返回文件描述符，出错返回-1
int creat(const char *path, unsigned short mode);

// 成功返回0，出错返回-1
int close (int fd);

// 成功返回值依赖于cmd，出错返回-1
int fcntl(int fd, int cmd, )

#endif