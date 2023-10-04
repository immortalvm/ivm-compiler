#include "outbyte.h"


static int output_digits(unsigned long uq, unsigned int base, unsigned int ucase, unsigned int prefix)
{
    char digit[20];
    char *upper = "0123456789ABCDEF";
    char *lower = "0123456789abcdef";
    char *glifo = (ucase)? upper: lower;
    int retval = 0;
    if (prefix) {
        if (base == 8) ivm64_outbyte('0');
        else if (base == 16) {
            ivm64_outbyte('0');
            ivm64_outbyte((ucase)? 'X': 'x');
        }
    }
    int i=0;
    if (base < 2) base = 2;
    else if (base > 16) base = 16;
    do {
       digit[i++] = glifo[uq % base];
    } while (uq /= base);
    while (i--) {
        ivm64_outbyte(digit[i]);
        retval++;
    }
    return retval;
}

// Easy access to varidic arguments
typedef void** va_var;
#define VA_INIT(varg, first) do{ varg = (void **)&first; (varg) && varg++; }while(0)
#define VA_ARG(varg, type) ({ type tmp = (varg)? *(type*)varg: (type)0; (varg) && varg++; tmp; })
#define VA_END(varg) do{ varg = NULL; }while(0)

// print formated string (no floats, no buffer,...)
// %d, %i           -> int (signed)
// %u, %o, %x, %X   -> unsigned int
// %ld, %li, %lu... -> [signed/unsigned] long
// %#x, %#X, %#o    -> prefix "0x", "0X" or "0"
// %s               -> string
// %c               -> char
int printk(const char *format,...)
{
   int retval = 0;           // return value, the number of characters output
   va_var arg;               // visits the variadics arguments
   char *p=(char *)format;   // visits format
   char c;                   // every char in format, string or place for a char argument
   char *t;                  // visits strings
   long q;                   // place for an argument that is a signed number
   unsigned long uq;         // place for an argument that is an unsigned numbers
   unsigned int base;        // base of representation (8, 10, 16)
   unsigned int ucase = 1;   // 0: lowercase; 1: uppercase letters in hexa numbers
   unsigned int islong = 0;  // fetch a 8 byte numbre rather than a 4 byte number
   unsigned int prefix = 0;  // prefix '0' for octal, and '0x' or '0X' for hexa depending on ucase

   VA_INIT(arg, format);
   while (0 != (c = *p++))
   {
      if (c == '%' || islong || prefix)
      {
         if (c == '%') c = *p++;
         switch (c)
         {
	 case 'l':
            islong = 1;
            break;
         case '#':
            prefix = 1;
            break;
         case 'd':
         case 'i':
            prefix = 0;
            base = 10;
            if (islong) q = VA_ARG(arg, long);
            else q = (long) VA_ARG(arg, int);
            islong = 0;
            // pass-through
         longint:
            if (q < 0) {
               uq = -q;
               ivm64_outbyte('-');
               retval++;
            } else {
               uq = q;
	    }
            retval += output_digits(uq, base, 0, prefix);
            break;
         case 'u':
            prefix = 0;
            base = 10;
            goto arg_unsigned;
         case 'o':
            base = 8;
            goto arg_unsigned;
         case 'p':
	    prefix = 1;
            islong = 1;
         case 'x':
            ucase = 0;
         case 'X':
            base = 16;
         arg_unsigned:
            if (islong) uq = VA_ARG(arg, unsigned long);
            else uq = (unsigned long) VA_ARG(arg, unsigned int);
            islong = 0;
            retval += output_digits(uq, base, ucase, prefix);
            ucase = 0;
            prefix = 0;
            break;
         case 's':
            t = VA_ARG(arg, unsigned char*);
            if (!t) t = "(null)";
            while (0 != (c = *t++)) {
               ivm64_outbyte(c);
               retval++;
            }
            islong = 0;
            prefix = 0;
            break;
         case 'c':
            c = VA_ARG(arg, unsigned char);
            // pass-through
         default:
            ivm64_outbyte(c);
            retval++;
            islong = 0;
            prefix = 0;
            ucase = 1;
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

