/*
 * header file to be used by applications.
 */

#ifndef _USER_LIB_H_
#define _USER_LIB_H_
#include "util/types.h"
#include "kernel/proc_file.h"

int printu(const char *s, ...);
int exit(int code);
void* naive_malloc();
void naive_free(void* va);
int fork();
void yield();

// added @ lab4_1
int open(const char *pathname, int flags);
int read_u(int fd, void *buf, uint64 count);
int write_u(int fd, void *buf, uint64 count);
int lseek_u(int fd, int offset, int whence);
int stat_u(int fd, struct istat *istat);
int disk_stat_u(int fd, struct istat *istat);
int close(int fd);

// added @ lab4_2
int opendir_u(const char *pathname);
int readdir_u(int fd, struct dir *dir);
int mkdir_u(const char *pathname);
int closedir_u(int fd);
int print_backtrace(int n);

// added @ lab4_3
int link_u(const char *fn1, const char *fn2);
int unlink_u(const char *fn);
int wait(int pid);
int exec(char *s,char *para);
//lab2-challenge2
void better_free(void* va);
void* better_malloc(int n);
//lab4-challenge1
int read_cwd(char *path);
int change_cwd(const char *path);
//lab3-challenge2
int sem_V(int a1);
int sem_new(int i);
int sem_P(int a1);
void printpa(int* va);
int gets(char *s);
int exec1(char *s,char *para);
#endif
