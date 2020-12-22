/* ivm64:  this is a distilled version of code 25_quicksort.s from tcc benchmark
           which didn't work with -O2
         The problem was caused by some optimization during the postreload
         pass that replaced the RTL operation (TR <- TR + something) with
         (TR <- AR) because (TR + something) was previously stored in AR.
         This transfer (TR <- AR) is valid insomuch it is solved by a given rule.
         By disabling it on postreload, the rule becomes disabled and
         the harmful transformation is not applied
         With this purpose, the instruction predicate !ivm64_postreload_in_progress()
         was added to rules "*mov<mode>_to_tr" "*mov<mode>_from_tr"
*/ 

#include <stdio.h>

#define TYPE_I long 

TYPE_I array[16];

//Partition the array into two halves and return the
//index about which the array is partitioned
TYPE_I partition(TYPE_I left, TYPE_I right)
{
   TYPE_I i=0;

   array[0] = array[left];
   for(i = left; i < right; i++)
   {
      if(array[i] < 62)
      {
        array[i] = array[left];
      }
   }
   array[0] = array[left];

   return left;
}


TYPE_I main()
{
   TYPE_I i;

   array[0] = 62;

   partition(0, 1);

   printf("OK\n");

   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
