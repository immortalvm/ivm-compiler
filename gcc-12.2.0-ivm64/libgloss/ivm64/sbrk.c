#ifdef __ivm64__

/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>


/* This global variables should be initialized by crt0 */
extern char* __IVM64_heap_pointer__;

/* Maximum number of allocated bytes allowed */
/* This global variable may be changed by ivm_spawn */
unsigned long __IVM64_max_heap_allocated__ = ULONG_MAX;

#define IVM64_STACK_SPARE (1024*64)
void* sbrk(intptr_t incr)
{
    static long allocated = 0; // Number of bytes already allocated
    void *sp = &incr;          // Stack pointer estimation

    // Limit check
    if ((allocated + incr) < 0 || (allocated + incr) > __IVM64_max_heap_allocated__){
        errno = ENOMEM;
        return (void *)(-1);
    }

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

#endif
