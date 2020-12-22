/* Static references to global variables */

/* A pattern like this should appear 
   in the data section:
	 p: data8 [(+ A 8)]
*/

char A[7] = "hello!";
char *p = &A[1];

char main(int c, char** v)
{
	return *p;
}
