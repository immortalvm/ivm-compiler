#include "outbyte.h"


// print formated string (no floats, no buffer,...)
int printk(const char *format,...)
{
   char digit[20];
   int retval=0;
   void **arg;
   char *p=(char *)format;
   char c, d;
   char *t;
   long q;
   unsigned long uq;
   char i;

   arg=(void **)&format;
   arg++;
   while (0!=(c=*p++))
   {
      if (c=='%')
      {
         c=*p++;
         switch (c)
         {
         case 'd':
         case 'u':
            q=(long) *arg++;
            if (q<0) {
               q=-q;
               ivm64_outbyte('-');
               retval++;
            }
            i=0;
            do {
               digit[i++]=(q%10)+'0';
            } while (q/=10);
            goto output_digits;
         case 'x':
            uq=(unsigned long) *arg++;
            i=0;
            do {
               d=(char)(uq&0xf);
               digit[i++]=(d>9)?d-10+'a':d+'0';
            } while (uq>>=4);
         output_digits:
            while (i--) {
                 c=digit[i];
                 ivm64_outbyte(c);
                 retval++;
            }
            break;
         case 's':
            t=*arg++;
            while (0!=(c=*t++)) {
               ivm64_outbyte(c);
               retval++;
            }
            break;
         default:
            ivm64_outbyte(c);
            retval++;
         }
      }
      else
      {
         ivm64_outbyte(c);
         retval++;
      }
   }
   return retval;
}

