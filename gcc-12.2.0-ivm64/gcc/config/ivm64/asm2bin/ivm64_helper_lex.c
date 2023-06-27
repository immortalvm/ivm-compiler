/* IVM64 assembler
   Helper functions for lexer
   Author: Eladio Gutierrez
           University of Malaga, Spain
   Jan 2022
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include "ivm64_helper.h"

#define BYTES_PER_WORD 8

#ifdef IVM64_LEX_AS_0

/* Evaluate numeric expression with ~ (not)
   or - (neg) prefixes, such as "--~-~~-1"
*/
uint64_t lex_parser_num(const char *s, int base){
    const char *p;
    uint64_t v = 0;
    if (!s) return 0; /* empty string */
    /* find the first number of the numerical part*/
    for (p=s; *p != 0; p++){
        if ( *p>='0' && *p<='9')
            break;
    }
    /* Keep the first - sign, in order to
       avoid arithmetic two's complement
       overflow, like for "-9223372036854775808" */
    if (p != s && p[-1] == '-') p--;
    /* scan the numerical part*/
    if (base == 10)
        sscanf(p, "%ld", &v);
    if (base == 16)
        sscanf(p, "%lx", &v);
    /* apply not or neg in reverse order*/
    p--;
    for (; p >= s; p--){
        if (*p == '-')
            v = -v;
        if (*p == '~')
            v = ~v;
    }
    return v;
}

#endif

#ifdef IVM64_LEX_AS_1
#endif

