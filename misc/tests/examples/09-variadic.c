/* Call to a variadic function
   This is necessary to implement functions such as printf()
*/

#include <stdarg.h>

int variadic(int n, ...){
   va_list args;
   va_start(args, n);	
   
   int s=0;
   for (int i=0; i<n; i++){
	   s += va_arg(args, int);
   }
   
   va_end(args);
   return s; 
}

int main(int c, char **v){
    return variadic(4, 0, 1, 2, 3);
}
