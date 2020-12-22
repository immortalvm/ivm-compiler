/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>

#include <signal.h>
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset){errno = ENOSYS; return -1;}

#include <sys/types.h>
#include <regex.h>
int regcomp(regex_t *preg, const char *regex, int cflags) {errno = ENOSYS; return -1;}
int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags) {errno = ENOSYS; return -1;}
void regfree(regex_t *preg){}

#include <stdlib.h>
int posix_memalign(void **memptr, size_t alignment, size_t size){errno = ENOSYS; return -1;}

#include <sys/stat.h>
#include <sys/types.h>
int mkdir(const char *pathname, mode_t mode){errno = ENOSYS; return -1;}

int getentropy(void *buffer, size_t length){errno = ENOSYS; return -1;}
