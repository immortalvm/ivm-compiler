#include <stdio.h>

extern int a, b;
int main() 
{
    printf("External: a=%d, b=%d, returning a+b=%d\n", a, b, a+b);
    return a+b;
}
