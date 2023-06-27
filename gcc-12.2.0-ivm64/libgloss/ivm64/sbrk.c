/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

/* This global variables should be initialized by crt0 */
extern char* __IVM64_heap_pointer__;

#define IVM64_STACK_SPARE (1024*64)
void* sbrk(intptr_t incr)
{
    static long allocated = 0; // Number of bytes already allocated
    void *sp = &incr; // Stack pointer estimation

    void* new_brk = (void *)((long)__IVM64_heap_pointer__ + allocated);

    int exhausted = 0;
    if ((unsigned long)sp >= (unsigned long)new_brk)
        exhausted = ((unsigned long)(new_brk) + incr
                     >= ((unsigned long)(sp) - IVM64_STACK_SPARE));

    if (exhausted) {
        errno = ENOMEM;
        return (void *)(-1);
    } else {
        allocated += incr;
        return new_brk;
    }
}
