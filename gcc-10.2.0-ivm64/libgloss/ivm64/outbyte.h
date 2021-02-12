// output char
#define ivm64_outbyte_const(c) ({asm volatile ("put_char! %0\n": : "i" (c));})
#define ivm64_outbyte_var(c) ({asm volatile ("put_char! (load1 %0)\n": :"m" (c));})

#define ivm64_outbyte(c)	\
  (__builtin_constant_p(c)	\
  ? ivm64_outbyte_const(c)	\
  : ivm64_outbyte_var(c))



/*
__attribute__((optimize("O0")))
__attribute__((noinline))
static int ivm64_outbyte(int arg)
{
    unsigned char ascii=arg;
    int retval=ascii;

    asm volatile ("load1! %0\n": "=m" (ascii));
    asm volatile ("put_char");
    return retval;
}
*/

/*
// print string -> better use printk()
static int _IVM64_puts(const char *str)
{
    int cont=0;
    char c;
    while (c = *str++){
         ivm64_outbyte(c);
         cont++;
    }
    ivm64_outbyte('\n');
    cont++;
    return cont;
}
*/
