/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

#include <setjmp.h>
jmp_buf *__IVM64_exit_jb__ = NULL;
unsigned long __IVM64_exit_val__ = 0;

void _exit(int status)
{
    if (__IVM64_exit_jb__){
     __IVM64_exit_val__ = status;
     __builtin_longjmp(*__IVM64_exit_jb__, 1);
    } else {
        asm volatile ("set_sp! %0"::"m"(status));
        asm volatile ("sigx4");
        asm volatile ("exit");
    }
    while(1){} // Avoid warning "'noreturn' function does return"
}
