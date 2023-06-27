// output char


#define ivm64_outbyte_const(c) ({asm volatile ("put_char! %0\n": : "i" (c));})
#define ivm64_outbyte_var(c) ({asm volatile ("put_char! (load1 %0)\n": :"m" (c));})

#define ivm64_outbyte(c)	\
  (__builtin_constant_p(c)	\
  ? ivm64_outbyte_const(c)	\
  : ivm64_outbyte_var(c))

