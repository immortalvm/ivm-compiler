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

int write(int fd, char *ptr, int nbytes)
{
    int cont;
    char c;
    for (cont=0; cont<nbytes; cont++){
         c = ptr[cont];
         ivm64_outbyte(c);
    }
    return cont;
}

