/*
# Preservation Virtual Machine Project
#
# setjmp.c - a sj/lj implementation for ivm64
#
# Authors:
#  Eladio Gutierrez Carrasco
#  Sergio Romero Montiel
#  Oscar Plata Gonzalez
#
# Date: May 2020
*/

#ifdef __ivm64__

#include <setjmp.h>
#include <stdio.h>

//__attribute__((optimize("O0")))
__attribute__ ((noinline))
__attribute__ ((returns_twice))
int setjmp(jmp_buf env){
    env[0].sp = (unsigned long)&env;
    env[0].pcret = *(unsigned long*)((unsigned long)&env - sizeof(unsigned long));
    return 0;
}

static void *j, *s;
static long v;

//__attribute__((optimize("O0")))
__attribute__ ((noinline))
__attribute__ ((noreturn))
void longjmp(jmp_buf env, int val)
{
    v = (val)?val:1; //  If the programmer mistakenly passes the value 0 in val, the "fake" return will instead return 1.

    s = (void *)(env[0].sp);
    j = (void *)env[0].pcret;

    asm volatile("load8! %0"::"m"(s));
    asm volatile("set_sp");

    asm volatile("store8!! (load8 %0) &0"::"m"(v));

    asm volatile("load8! %0"::"m"(j));
    asm volatile("jump");

    while(1){} // Avoid warning "'noreturn' function does return"
}

#endif
