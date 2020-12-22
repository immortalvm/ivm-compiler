/* Testing printf and manipulating strings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

char *s="Hello world!";

int main(){
    int c=0;
	char *p = malloc(256);
	char q[256];

    strncpy(p, s, 256);
	c += printf("%s\n", p);

    #define BYTES_PER_CHAR (CHAR_BIT>>3)
    memcpy(q, p, (strlen(p)+1)*BYTES_PER_CHAR);
    c += printf(" Again, %s\n", q);    
    c += printf(" strlen(\"%s\")=%d\n", p, strlen(p));

	sprintf(p, "   %s %d(=0x%x)\n", "No. of chars printed so far:", c, c);
	printf("%s", p);
	
    return c; 
}
