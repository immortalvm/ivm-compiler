%{
    /* IVM64 assembler
       Lexer phase 0 (for sugared assembly)
       Author: Eladio Gutierrez
               University of Malaga, Spain
       Jan 2022
    */

    #include  <stdio.h>
    #include  <stdlib.h>
    #include  <string.h>
    #include "y.tab.0.h"  /* Do not forget to define the bison output file:
                             EITHER with "%output y.tab.0.c" and "%defines y.tab.0.h" (not POSIX),
                             OR compile with "bison -o y.tab.0.c"*/

    int noerror = 1; /* To be used by bison */

    #define IVM64_LEX_AS_0
    #include "ivm64_helper_lex.c"
%}

COM    "#"[^\n\r]*
WS     [ \r\n\t]

HEX    -{0,1}0[Xx][0-9A-Fa-f]+
DEC    -{0,1}[0-9]+

%{
/* Some definitions for unicode support in labels*/
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

%{
/* Labels can appear negated or with not (-label, ~label) */
%}

LABEL_START_CHAR  ([A-Za-z_\.]|{UONLY})
LABEL_CONT_CHAR   ([A-Za-z0-9_\.]|{UONLY})

LABEL_NAME {LABEL_START_CHAR}{LABEL_CONT_CHAR}*
LABEL      {LABEL_NAME}
LABEL_DECL {LABEL_NAME}{WS}*:

%{
/* Stack operands (not/neg sign can precede the number) */
%}
STACKOPA "&"[-~]*{DEC}
STACKOPD "$"[-~]*{DEC}
STACKOPA_HEX "&"[-~]*{HEX}
STACKOPD_HEX "$"[-~]*{HEX}

%option yylineno

%%
"(" {return '(';}
")" {return ')';}
"[" {return '[';}
"]" {return ']';}
"!" {return '!';}
"+" {return '+';}
"*" {return '*';}
"&" {return '&';}
"$" {return '$';}
"|" {return '|';}
"^" {return '^';}
"-" {return '-';}
"~" {return '~';}
"=" {return '=';}

"push"      {return TOKEN_PUSH;}

"set_sp"        {return TOKEN_SET_SP;}
"jump"          {return TOKEN_JUMP;}
"jump_zero"     {return TOKEN_JUMP_ZERO;}
"call"          {return TOKEN_CALL;}
"return"        {return TOKEN_RETURN;}
"jump_not_zero" {return TOKEN_JUMP_NOT_ZERO;}

"load8"     {return TOKEN_LOAD8;}
"load4"     {return TOKEN_LOAD4;}
"load2"     {return TOKEN_LOAD2;}
"load1"     {return TOKEN_LOAD1;}

"sigx8"     {return TOKEN_SIGX8;}
"sigx4"     {return TOKEN_SIGX4;}
"sigx2"     {return TOKEN_SIGX2;}
"sigx1"     {return TOKEN_SIGX1;}

"store8"    {return TOKEN_STORE8;}
"store4"    {return TOKEN_STORE4;}
"store2"    {return TOKEN_STORE2;}
"store1"    {return TOKEN_STORE1;}

"add"       {return TOKEN_ADD;}
"sub"       {return TOKEN_SUB;}
"mult"      {return TOKEN_MULT;}
"neg"       {return TOKEN_NEG;}
"and"       {return TOKEN_AND;}
"or"        {return TOKEN_OR;}
"xor"       {return TOKEN_XOR;}
"not"       {return TOKEN_NOT;}
"pow2"      {return TOKEN_POW2;}
"check_version"   {return TOKEN_CHECK_VERSION;}
"shift_l"   {return TOKEN_SHIFT_L;}
"shift_ru"  {return TOKEN_SHIFT_RU;}
"shift_rs"  {return TOKEN_SHIFT_RS;}
"div_u"     {return TOKEN_DIV_U;}
"div_s"     {return TOKEN_DIV_S;}
"rem_u"     {return TOKEN_REM_U;}
"rem_s"     {return TOKEN_REM_S;}
"lt_u"      {return TOKEN_LT_U;}
"lt_s"      {return TOKEN_LT_S;}
"lte_u"     {return TOKEN_LTE_U;}
"lte_s"     {return TOKEN_LTE_S;}
"eq"        {return TOKEN_EQ;}
"gte_u"     {return TOKEN_GTE_U;}
"gte_s"     {return TOKEN_GTE_S;}
"gt_u"      {return TOKEN_GT_U;}
"gt_s"      {return TOKEN_GT_S;}
"read_frame" {return TOKEN_READ_FRAME;}
"read_pixel" {return TOKEN_READ_PIXEL;}
"read_char"  {return TOKEN_READ_CHAR;}
"put_char"  {return TOKEN_PUT_CHAR;}
"put_byte"  {return TOKEN_PUT_BYTE;}
"new_frame" {return TOKEN_NEW_FRAME;}
"set_pixel" {return TOKEN_SET_PIXEL;}
"add_sample" {return TOKEN_ADD_SAMPLE;}

"exit"      {return TOKEN_EXIT;}

"data8"     {return TOKEN_DATA8;}
"data4"     {return TOKEN_DATA4;}
"data2"     {return TOKEN_DATA2;}
"data1"     {return TOKEN_DATA1;}

"space"     {return TOKEN_SPACE;}
"EXPORT"    {return TOKEN_EXPORT;}
"IMPORT"    {return TOKEN_IMPORT;}

"<u"  {return TOKEN_EXPR_LT_U;}
"<s"  {return TOKEN_EXPR_LT_S;}
"<=u" {return TOKEN_EXPR_LTE_U;}
"<=s" {return TOKEN_EXPR_LTE_S;}
">u"  {return TOKEN_EXPR_GT_U;}
">s"  {return TOKEN_EXPR_GT_S;}
">=u" {return TOKEN_EXPR_GTE_U;}
">=s" {return TOKEN_EXPR_GTE_S;}
"<<"  {return TOKEN_EXPR_SHIFT_L;}
">>s" {return TOKEN_EXPR_SHIFT_RS;}
">>u" {return TOKEN_EXPR_SHIFT_RU;}
"/u"  {return TOKEN_EXPR_DIV_U;}
"/s"  {return TOKEN_EXPR_DIV_S;}
"%u"  {return TOKEN_EXPR_REM_U;}
"%s"  {return TOKEN_EXPR_REM_S;}


{WS}+        { /* skip white spaces */ }
{COM}        { /* skip comments */ }
[-~]*{DEC}   {
              yylval.num = lex_parser_num(yytext, 10);
              return TOKEN_NUM;
             }
[-~]*{HEX}   {
              yylval.num = lex_parser_num(yytext, 16);
              return TOKEN_NUM;
             }
{LABEL}      {
              /* It's important that we copy the string here,
               * because the `yytext` pointer will get reused
               * as the lexer continues. */
              yylval.label= strdup(yytext);
              return TOKEN_LABEL;
             }
{LABEL_DECL} {
              /* Remove ':' from label declaration */
              yytext[yyleng-1] = '\0';
              /* Remove extra spaces from label declaration */
              yylval.label= strdup(trim(yytext));
              return TOKEN_LABEL_DECL;
             }
{STACKOPA}  {
              yylval.num = lex_parser_num(&yytext[1], 10);
              return TOKEN_STACK_OP_ADDR;
            }

{STACKOPD}  {
              yylval.num = lex_parser_num(&yytext[1], 10);
              return TOKEN_STACK_OP_VALUE;
            }
{STACKOPA_HEX}  {
              yylval.num = lex_parser_num(&yytext[1], 16);
              return TOKEN_STACK_OP_ADDR;
            }

{STACKOPD_HEX}  {
              yylval.num = lex_parser_num(&yytext[1], 16);
              return TOKEN_STACK_OP_VALUE;
            }

.|\n {fprintf(stderr, "Unknown token %s\n", yytext); return TOKEN_UNKNOWN;}
%%

/* In the event of some kind of error, this will get called. */
void yyerror(const char* msg)
{
    fprintf(stderr, "as0 lexer error: %s in line %d\n", msg, yylineno);
    noerror = 0;
}

