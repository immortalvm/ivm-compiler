/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>

#include "outbyte.h"

off_t lseek(int fd,  off_t offset, int whence) { errno = ESPIPE; return ((off_t)-1); }
int fcntl (int fd, int cmd, ...) { errno = EINVAL; return -1; }

