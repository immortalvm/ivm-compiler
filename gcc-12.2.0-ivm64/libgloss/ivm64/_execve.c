/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>

/* execve() provided by "libc/posix/execve.c" by calling _execve() */
int _execve(char *name, char **argv, char **env) {errno = ENOSYS; return -1;}


