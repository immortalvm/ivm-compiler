%{
    /* IVM64 assembly preprocessor
       Alias preprocessing phase 1
       Author: Eladio Gutierrez
               University of Malaga, Spain
       Jan 2022
    */

    #include  <stdio.h>
    #include  <stdlib.h>
    #include  <string.h>
    #include  "y.tab.1.h"  /* Do not forget to define the bison output file:
                             EITHER with "%output y.tab.1.c" and "%defines y.tab.1.h" (not POSIX),
                             OR compile with "bison -o y.tab.1.c"*/
    #include  "../ivm64_helper.h"

    int noerror = 1; /* To be used by bison */
%}

COM    "#"[^\n\r]*
WS     [ \t\r\n]

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
/* Valid labels do not start by a number*/
%}
LABEL_START_CHAR  ([A-Za-z_\.]|{UONLY})
LABEL_CONT_CHAR   ([A-Za-z0-9_\.]|{UONLY})
LABEL             {LABEL_START_CHAR}{LABEL_CONT_CHAR}*

LABEL_DECL        {LABEL}{WS}*:

%{
/* Unsigned numbers */
%}
HEX    0[Xx][0-9A-Fa-f]+
DEC    [0-9]+

%option yylineno

%s CODE

%%
{COM}        { /* skip comments */ }
{LABEL}"="[ \t]*[^#\n\r]+/[#\n\r] {
                               yylval.str = strdup(yytext);
                               return TOKEN_ALIAS_DECL;
                               }

<INITIAL>{WS}+ {
        yylval.str = strdup(yytext);
        return TOKEN_WS;
      }

{LABEL_DECL} {
              BEGIN(CODE);
              /* Remove ':' from label declaration */
              yytext[yyleng-1] = '\0';
              /* Remove extra spaces from label declaration */
              yylval.str = strdup(trim(yytext));
              return TOKEN_LABEL_DECL;
             }

{LABEL} {
         BEGIN(CODE);
         yylval.str = strdup(yytext);
         return TOKEN_LABEL;
        }

<CODE>{WS}+ {
        yylval.str = strdup(yytext);
        return TOKEN_WS_CODE;
      }

<CODE>{DEC}|{HEX} {
             /* This pattern avoid replace aliases in hex. numbers
                (xa=33 push!! 000xa); also it is more efficient*/
             yylval.str = strdup(yytext);
             return TOKEN_NUMBER;
            }

<CODE>[^0-9A-Za-z_\.\x80-\xf4\n\r]+ {
      /* This pattern (sequence of chars that are not valid
         for labels, nor newlines) is for efficiency */
      yylval.str = strdup(yytext);
      return TOKEN_OTHER;
}

.|\n {
      BEGIN(CODE);
      yylval.str = strdup(yytext);
      return TOKEN_OTHER;
     }
%%

/* In the event of some kind of error, this will get called. */
void yyerror(const char* msg)
{
    fprintf(stderr, "aspp1 lexer error: %s in line %d\n", msg, yylineno);
    noerror = 0;
}

