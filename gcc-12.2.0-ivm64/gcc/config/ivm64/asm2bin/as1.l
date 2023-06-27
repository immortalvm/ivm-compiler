%{
    /* IVM64 assembler
       Lexer phase 1 (for native assembly from phase 0)
       Author: Eladio Gutierrez
               University of Malaga, Spain
       Jan 2022
    */

    #include  <stdio.h>
    #include  <stdlib.h>
    #include  <string.h>
    #include "y.tab.1.h"  /* Do not forget to define the bison output file:
                             EITHER with "%output y.tab.1.c" and "%defines y.tab.1.h" (not POSIX),
                             OR compile with "bison -o y.tab.1.c"*/

    #define IVM64_LEX_AS_1
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


"exit"      {yylval.opcode = OPCODE_EXIT     ; return TOKEN_EXIT;}
"nop"       {yylval.opcode = OPCODE_NOP      ; return TOKEN_NOP;}
"jump"      {yylval.opcode = OPCODE_JUMP     ; return TOKEN_JUMP;}
"jzfwd"     {yylval.opcode = OPCODE_JZFWD    ; return TOKEN_JZFWD;}
"jzback"    {yylval.opcode = OPCODE_JZBACK   ; return TOKEN_JZBACK;}
"setsp"     {yylval.opcode = OPCODE_SETSP    ; return TOKEN_SETSP;}
"getpc"     {yylval.opcode = OPCODE_GETPC    ; return TOKEN_GETPC;}
"getsp"     {yylval.opcode = OPCODE_GETSP    ; return TOKEN_GETSP;}
"push0"     {yylval.opcode = OPCODE_PUSH0    ; return TOKEN_PUSH0;}
"push1"     {yylval.opcode = OPCODE_PUSH1    ; return TOKEN_PUSH1;}
"push2"     {yylval.opcode = OPCODE_PUSH2    ; return TOKEN_PUSH2;}
"push4"     {yylval.opcode = OPCODE_PUSH4    ; return TOKEN_PUSH4;}
"push8"     {yylval.opcode = OPCODE_PUSH8    ; return TOKEN_PUSH8;}
"load1"     {yylval.opcode = OPCODE_LOAD1    ; return TOKEN_LOAD1;}
"load2"     {yylval.opcode = OPCODE_LOAD2    ; return TOKEN_LOAD2;}
"load4"     {yylval.opcode = OPCODE_LOAD4    ; return TOKEN_LOAD4;}
"load8"     {yylval.opcode = OPCODE_LOAD8    ; return TOKEN_LOAD8;}
"store1"    {yylval.opcode = OPCODE_STORE1   ; return TOKEN_STORE1;}
"store2"    {yylval.opcode = OPCODE_STORE2   ; return TOKEN_STORE2;}
"store4"    {yylval.opcode = OPCODE_STORE4   ; return TOKEN_STORE4;}
"store8"    {yylval.opcode = OPCODE_STORE8   ; return TOKEN_STORE8;}
"add"       {yylval.opcode = OPCODE_ADD      ; return TOKEN_ADD;}
"mult"      {yylval.opcode = OPCODE_MULT     ; return TOKEN_MULT;}
"div"       {yylval.opcode = OPCODE_DIV      ; return TOKEN_DIV;}
"rem"       {yylval.opcode = OPCODE_REM      ; return TOKEN_REM;}
"lt"        {yylval.opcode = OPCODE_LT       ; return TOKEN_LT;}
"and"       {yylval.opcode = OPCODE_AND      ; return TOKEN_AND;}
"or"        {yylval.opcode = OPCODE_OR       ; return TOKEN_OR;}
"not"       {yylval.opcode = OPCODE_NOT      ; return TOKEN_NOT;}
"xor"       {yylval.opcode = OPCODE_XOR      ; return TOKEN_XOR;}
"pow"       {yylval.opcode = OPCODE_POW      ; return TOKEN_POW;}
"check"     {yylval.opcode = OPCODE_CHECK    ; return TOKEN_CHECK;}
"putbyte"   {yylval.opcode = OPCODE_PUTBYTE  ; return TOKEN_PUTBYTE;}
"readchar"  {yylval.opcode = OPCODE_READCHAR ; return TOKEN_READCHAR;}
"putchar"   {yylval.opcode = OPCODE_PUTCHAR  ; return TOKEN_PUTCHAR;}
"addsample" {yylval.opcode = OPCODE_ADDSAMPLE; return TOKEN_ADDSAMPLE;}
"setpixel"  {yylval.opcode = OPCODE_SETPIXEL ; return TOKEN_SETPIXEL;}
"newframe"  {yylval.opcode = OPCODE_NEWFRAME ; return TOKEN_NEWFRAME;}
"readpixel" {yylval.opcode = OPCODE_READPIXEL; return TOKEN_READPIXEL;}
"readframe" {yylval.opcode = OPCODE_READFRAME; return TOKEN_READFRAME;}


"data8"     {yylval.num=8; return TOKEN_DATA8;}
"data4"     {yylval.num=4; return TOKEN_DATA4;}
"data2"     {yylval.num=2; return TOKEN_DATA2;}
"data1"     {yylval.num=1; return TOKEN_DATA1;}


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
              yylval.label= strdup(&yytext[1]); /* Remove '%'*/
              return TOKEN_LABEL;
            }
"<"{LABEL}  {
              /* This is a 'near label', which can be an
                 argument of 'jzfwd' when it is known
                 that its offset fills in one byte (short jumps)*/
              yylval.label= strdup(&yytext[1]); /* Remove '<'*/
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
              return TOKEN_ALIAS_FROM;
             }
{ALIAS_DECL_MNEMO} {
              yytext[yyleng-1] = '\0'; /* Remove '=' at the end */
              char *s = trim(yytext);  /* Remove blanks */
              yylval.label= strdup(&yytext[1]); /* Remove '%'*/
              return TOKEN_ALIAS_FROM;
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

