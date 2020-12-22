#include <stdio.h>

int myfunc(int x)
{
   return x * x;
}

void vfunc(int a)
{
   printf("a=%d\n", a);
}

void qfunc()
{
   //printf("qfunc()\n");
   printf("qfunc()%d\n", 0); /*e Added an argument to prevent printf() from being replaced by puts()*/
}

void zfunc()
{
   ((void (*)(void))0) ();
}

int main()
{
   printf("%d\n", 3);
   //printf("%d\n", myfunc(3));
   //printf("%d\n", myfunc(4));

   //vfunc(1234);

   //qfunc();

   return 0;
}

// vim: set expandtab ts=4 sw=3 sts=3 tw=80 :
