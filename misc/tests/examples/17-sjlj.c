// Use this code to test sj/lj
// From https://www.embecosm.com/appnotes/ean9/ean9-howto-newlib-1.0.html#sec_setjmp_longjmp

#include <stdio.h>
#include <setjmp.h>

void
testit (jmp_buf  env,
        int      prev_res)
{
  int  res = (0 == prev_res) ? prev_res : prev_res + 1;

  printf ("Long jumping with result %d\n", res);
  longjmp (env, res);

}       /* testit () */


int
main (int   argc,
      char *argv[])
{
  jmp_buf  env;

  int  res = setjmp (env);

  printf ("res = 0x%08x\n", res);

  if (res > 1)
    {
      return  0;
    }

  testit (env, res);

  return  256;                  /* We should never actually get here */

}       /* main () */
