#include <stdio.h>
#include <stdlib.h>

#define N 11

union {
    double        f; 
    unsigned long i; 
  } u;

float z[N];

int main(){
    volatile double x;
    volatile double y;

    x=3.14159;
    y=125; 

    z[N-1] = (x/y+y/x);
    z[N-1] = (x*x + 2*y*y - 3*z[N-1]*z[N-1]); 

    u.f = z[N-1];
    u.i = ~u.i;
    z[N-1] = u.f;

    printf("x=%e, y=%e\n" "z[%d]=%.4e, 1/z[%d]=%.4e\n", 
              x,    y,       N-1,z[N-1],   N-1,1/z[N-1]);

    #define ABS(x)   ((x>0)?(x):-(x))
    #define MAX(x,y) ((x>y)?(x):(y))

    return ((int)MAX(ABS(z[N-1] + 1/z[N-1]), ABS(z[N-1])))%251; 
}
