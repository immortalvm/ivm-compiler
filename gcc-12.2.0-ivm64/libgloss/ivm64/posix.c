/* note these headers are all provided by newlib - you don't need to provide them */
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>

// These functions related to posix regular expression are included in newlib/libc/posix
//~  #include <sys/types.h>
//~  #include <regex.h>
//~  int regcomp(regex_t *preg, const char *regex, int cflags) {errno = ENOSYS; return -1;}
//~  int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags) {errno = ENOSYS; return -1;}
//~  void regfree(regex_t *preg){}

// These functions are not found in newlib/libc/posix
#include <signal.h>
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset){errno = ENOSYS; return -1;}

#include <stdlib.h>
int posix_memalign(void **memptr, size_t alignment, size_t size){errno = ENOSYS; return -1;}

#include <sys/stat.h>
#include <sys/types.h>
int mkdir(const char *pathname, mode_t mode){errno = ENOSYS; return -1;}
int getentropy(void *buffer, size_t length){errno = ENOSYS; return -1;}

int chdir(const char *path) {return -1;}
int fchdir(int fd) {return -1;}
int chmod(const char *pathname, mode_t mode) {return -1;}
int fchmod(int fd, mode_t mode){return -1;}
int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags){return -1;}
long fpathconf(int fd, int name) {return -1;}
long pathconf(const char *path, int name) {return -1;}
char *getcwd(char *buf, size_t size){return NULL;}
char *get_current_dir_name(void) {return NULL;}

int ftruncate (int file, off_t length) {errno = ENOSYS; return -1;}
int truncate (const char *path, off_t length) { errno = ENOSYS; return -1;}

#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem) {return 0;}

#include <dirent.h>
long getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count){errno = ENOENT; return -1;}
int openat(int dirfd, const char *pathname, int flags, ...) {errno=EACCES; return -1;}
int unlinkat(int dirfd, const char *pathname, int flags){errno=EACCES; return -1;}
