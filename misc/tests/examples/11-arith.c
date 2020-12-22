/* Playing with integer arithmetic */
/* Note that in the current version, arithmetic 
   operations are implemented with function calls
*/

#include <stdio.h>

volatile int n = 4;
volatile int w= -1, x = 31, y = -150, z = 2;

int main(){
    int out = -(x*-3*y*x*(x%n)-y*y)/(2-w-z+y) + 
               (x>>n)/(z<<n) - (x%z);

    printf("out=%d\n", out);
    return 0;
}
