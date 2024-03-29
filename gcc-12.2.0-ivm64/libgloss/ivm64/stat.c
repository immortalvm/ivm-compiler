/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>

int stat (const char  *file, struct stat *st)
{
  errno = ENOSYS;
  return -1;
}

int lstat(const char *pathname, struct stat *st)
{
    stat(pathname, st);
}

