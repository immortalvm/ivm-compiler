#include <stdio.h>
#include <stdarg.h>

typedef  int (*fv_t)(const char *,...) ;

fv_t foo(void *first, ...){
    va_list args;
    va_start(args, first);   
    return va_arg(args, fv_t);
}

int main(){
    return foo(NULL, printf)("hello\n");
}
