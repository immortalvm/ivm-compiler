/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>

#undef errno
extern int errno;

int chown (const char *path, uid_t owner, gid_t group)
{
  errno = ENOSYS;
  return -1;
}
