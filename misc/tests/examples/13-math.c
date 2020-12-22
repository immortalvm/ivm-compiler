/* Compile with -lm */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

volatile double x=3.14e3, y=2.71;

int main(int argc, char *argv[])
{
    volatile float sx, cx; 
    volatile double j0x, j1x;

    x+=argc;
    y+=argc;

    sx = sin(x);
    cx = cos(x);

    j0x = j0(x/(1+x));
    j1x = j1(x/(1+x));

    int exp;
    double mantissa;
    mantissa = frexp(x, &exp);

    printf("mantissa(x)=%f, exp(x)=%d\n", mantissa, exp);
    printf("x=%e, sin(x)=%.4e, cos(x)=%.4e, s^2+c^2=%.1f\n", 
              x, sx, cx, pow(sx,2.0)+pow(cx,2.0) );
    printf("x=%e, bessel j0(x/(1+x))=%.4f, bessel j1(x/(1+x))=%.4f\n", 
              x, j0x, j1x);
    printf("log2(1<<47)=%.1f\n", log2(y/1.0));
    printf("0/0=%f\n", +0.0/+0.0*1.0);

    return 126;
}
