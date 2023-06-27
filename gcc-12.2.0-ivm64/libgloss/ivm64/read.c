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

static char ivm64_getbyte()
{
    static unsigned long res;
    asm volatile("read_char\n\t"
                 "store8! %0":"=m"(res));
    return (unsigned char)res;
}

// Which char is considered EOF (^D = 4)
#define SYSTEM_EOF 4

int read(int file, char *ptr, int nbytes)
{
    unsigned char ch = ivm64_getbyte();
    if (ch == SYSTEM_EOF){
        return 0;
    }
    ptr[0] = ch;
    return 1;
}


