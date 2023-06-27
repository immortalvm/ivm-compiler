%{
    /* IVM64 assembler
       Lexer for optimization phase (for native assembly from phase 0)
       Author: Eladio Gutierrez
               University of Malaga, Spain
       Jan 2023
    */

    #include  <stdio.h>
    #include  <stdlib.h>
    #include  <string.h>
    #include "y.tab.opt1.h"  /* Do not forget to define the bison output file:
                             EITHER with "%output y.tab.opt1.c" and "%defines y.tab.opt1.h" (not POSIX),
                             OR compile with "bison -o y.tab.opt1.c"*/

    #define IVM64_LEX_AS_OPT1
    #include "ivm64_helper_lex.c"

    int noerror = 1;
%}

COM    "#"[^\n\r]*
WS     [ \r\n\t]

HEX    0[Xx][0-9A-Fa-f]+
DEC    -{0,1}[0-9]+

%{
/* Some definitions for unicode support in labels
   See: https://stackoverflow.com/questions/9611682/flexlexer-support-for-unicode
        http://www.kylheku.com/cgit/txr/tree/parser.l */
%}
ASC     [\x00-\x7f]
ASCN    [\x00-\t\v-\x7f]
U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]
UANY    {ASC}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UANYN   {ASCN}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UONLY   {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

LABEL_START_CHAR  ([A-Za-z_\.]|{UONLY})
LABEL_CONT_CHAR   ([A-Za-z0-9_\.]|{UONLY})

LABEL_NAME {LABEL_START_CHAR}{LABEL_CONT_CHAR}*
LABEL      {LABEL_NAME}
LABEL_DECL {LABEL_NAME}{WS}*:

%{
  /* This represents the first part of an alias declaration,
     i.e, a line starting with "label =".
     Note that only the label is returned, removing
     sourounding blanks, and the '=' symbol.
     Aliases must be declared in one line. */
%}
ALIAS_DECL ^{WS}*{LABEL}{WS}*"="
%{
  /* Same as above for a mnemonic used as label
     e.g. "push8=3"
     (marked by as0 as starting with '%') */
%}
ALIAS_DECL_MNEMO ^{WS}*\%{LABEL}{WS}*"="

%{
  /* For the optimizer, data sentences are processes as a whole */
%}
DATA_DIRECTIVE   "data"("1"|"2"|"4"|"8")
DATA_DECL        {DATA_DIRECTIVE}{WS}*"["{WS}*({HEX}|{DEC}){WS}*"]"

%option yylineno

/* In the below code, the TOKEN_blah types and the yylval
 * structure are used; these are defined in parser.y */

%%

"(" {return '(';}
")" {return ')';}
"[" {return '[';}
"]" {return ']';}
"!" {return '!';}
"+" {return '+';}
"*" {return '*';}
"&" {return '&';}
"|" {return '|';}
"^" {return '^';}
"-" {return '-';}
"~" {return '~';}
"=" {return '=';}


"exit"      {yylval.label = strdup(yytext); return TOKEN_EXIT;}
"nop"       {yylval.label = strdup(yytext); return TOKEN_NOP;}
"jump"      {yylval.label = strdup(yytext); return TOKEN_JUMP;}
"jzfwd"     {yylval.label = strdup(yytext); return TOKEN_JZFWD;}
"jzback"    {yylval.label = strdup(yytext); return TOKEN_JZBACK;}
"setsp"     {yylval.label = strdup(yytext); return TOKEN_SETSP;}
"getpc"     {yylval.label = strdup(yytext); return TOKEN_GETPC;}
"getsp"     {yylval.label = strdup(yytext); return TOKEN_GETSP;}
"push0"     {yylval.label = strdup(yytext); return TOKEN_PUSH0;}
"push1"     {yylval.label = strdup(yytext); return TOKEN_PUSH1;}
"push2"     {yylval.label = strdup(yytext); return TOKEN_PUSH2;}
"push4"     {yylval.label = strdup(yytext); return TOKEN_PUSH4;}
"push8"     {yylval.label = strdup(yytext); return TOKEN_PUSH8;}
"load1"     {yylval.label = strdup(yytext); return TOKEN_LOAD1;}
"load2"     {yylval.label = strdup(yytext); return TOKEN_LOAD2;}
"load4"     {yylval.label = strdup(yytext); return TOKEN_LOAD4;}
"load8"     {yylval.label = strdup(yytext); return TOKEN_LOAD8;}
"store1"    {yylval.label = strdup(yytext); return TOKEN_STORE1;}
"store2"    {yylval.label = strdup(yytext); return TOKEN_STORE2;}
"store4"    {yylval.label = strdup(yytext); return TOKEN_STORE4;}
"store8"    {yylval.label = strdup(yytext); return TOKEN_STORE8;}
"add"       {yylval.label = strdup(yytext); return TOKEN_ADD;}
"mult"      {yylval.label = strdup(yytext); return TOKEN_MULT;}
"div"       {yylval.label = strdup(yytext); return TOKEN_DIV;}
"rem"       {yylval.label = strdup(yytext); return TOKEN_REM;}
"lt"        {yylval.label = strdup(yytext); return TOKEN_LT;}
"and"       {yylval.label = strdup(yytext); return TOKEN_AND;}
"or"        {yylval.label = strdup(yytext); return TOKEN_OR;}
"not"       {yylval.label = strdup(yytext); return TOKEN_NOT;}
"xor"       {yylval.label = strdup(yytext); return TOKEN_XOR;}
"pow"       {yylval.label = strdup(yytext); return TOKEN_POW;}
"check"     {yylval.label = strdup(yytext); return TOKEN_CHECK;}
"putbyte"   {yylval.label = strdup(yytext); return TOKEN_PUTBYTE;}
"readchar"  {yylval.label = strdup(yytext); return TOKEN_READCHAR;}
"putchar"   {yylval.label = strdup(yytext); return TOKEN_PUTCHAR;}
"addsample" {yylval.label = strdup(yytext); return TOKEN_ADDSAMPLE;}
"setpixel"  {yylval.label = strdup(yytext); return TOKEN_SETPIXEL;}
"newframe"  {yylval.label = strdup(yytext); return TOKEN_NEWFRAME;}
"readpixel" {yylval.label = strdup(yytext); return TOKEN_READPIXEL;}
"readframe" {yylval.label = strdup(yytext); return TOKEN_READFRAME;}



{WS}+       { /* skip white spaces */ }
{COM}       { /* skip comments */ }
{HEX}       {
              sscanf(yytext, "%lx", &yylval.num);
              return TOKEN_NUM;
            }
{DEC}       {
              sscanf(yytext, "%ld", &yylval.num);
              return TOKEN_NUM;
            }
{LABEL}     {
              /* It's important that we copy the string here,
               * because the `yytext` pointer will get reused
               * as the lexer continues. */
              yylval.label= strdup(yytext);
              return TOKEN_LABEL;
            }
\%{LABEL}   {
              /* This is a mnemonic used as label
                 which was marked starting with '%' */
              yylval.label= strdup(&yytext[0]); /* IN THIS OPTIMIZATION STAGE do NOT remove '%'*/
              return TOKEN_LABEL;
            }
"<"{LABEL}  {
              /* This is a 'near label', which can be an
                 argument of 'jzfwd' when it is known
                 that its offset fills in one byte (short jumps)*/
              yylval.label= strdup(&yytext[0]); /* IN THIS OPTIMIZATION STAGE do NOT remove '<'*/
              return TOKEN_NEAR_LABEL;
            }
{LABEL_DECL} {
              /* Remove ':' from label declaration */
              yytext[yyleng-1] = '\0';
              /* Remove extra spaces from label declaration */
              yylval.label= strdup(trim(yytext));
              return TOKEN_LABEL_DECL;
             }
{ALIAS_DECL} {
              yytext[yyleng-1] = '\0'; /* Remove '=' at the end */
              yylval.label= strdup(trim(yytext)); /* Remove blanks */
              return TOKEN_ALIAS;
             }
{ALIAS_DECL_MNEMO} {
              yytext[yyleng-1] = '\0'; /* Remove '=' at the end */
              char *s = trim(yytext);  /* Remove blanks */
              yylval.label= strdup(&yytext[0]); /* IN THIS OPTIMIZATION STAGE do NOT remove '<'*/
              return TOKEN_ALIAS;
            }
{DATA_DECL} {
              yylval.label= strdup(yytext);
              return TOKEN_DATA;
            }

.|\n {fprintf(stderr, "Unknown token %s\n", yytext); return TOKEN_UNKNOWN;}
%%

/* In the event of some kind of error, this will get called. */
void yyerror(const char* msg)
{
    fprintf(stderr, "as1 lexer error: %s in line %d\n", msg, yylineno);
    noerror = 0;
    exit(EXIT_FAILURE);
}

