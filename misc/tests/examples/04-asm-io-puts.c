/* Illustrating the use of inline assembly for input/output */
/* Control structures can be also implemented in C language
   instead of raw assembly */

#ifdef __ivm64__
int ivm64_putch(int ch)
{
    asm volatile("load1! %0"::"m"(ch));
    asm volatile("put_char");
    return ch;
}

int ivm64_puts(const char *str)
{
    char c;

    // Strategy for stack unbalance
    const char* str_[1] = {str};

loop:
    c = str[0];

    // if (c==0) goto end
    asm volatile("load1! %0":: "m"(c));     
    asm goto    ("jump_zero! %l0"::::end);

    ivm64_putch(c);
    
    asm volatile("load8! %0":: "m"(str));
    asm volatile("add!  1");

    // NO C INSTRUCTION CAN BE PLACED 
    // WHILE THE STACK IS UNBALANCED

    // be aware! stack has now one element
    asm volatile("store8! %0": "=m"(str_[1]));
    str = str_[0]; 

    goto loop;
end:
    ivm64_putch(10);

    return 0;
}
#else
    // For targets other than ivm64 (e.g. x86)
    #include <stdio.h>
    #define ivm64_puts puts
#endif

int main()
{
    char *s="History Begins at Sumer";
    ivm64_puts(s);
    return 126;
}
