%{
    /* IVM64 assembly preprocessor
       Alias preprocessing phase 0
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

%}

COM    "#"[^\n\r]*
WS     [ \n\r\t]

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

%{
/* Valid labels do not start by a number*/
%}
LABEL_START_CHAR  ([A-Za-z_\.]|{UONLY})
LABEL_CONT_CHAR   ([A-Za-z0-9_\.]|{UONLY})
LABEL             {LABEL_START_CHAR}{LABEL_CONT_CHAR}*

%{
/* Alias declaration MUST be always in one only line
   and it ends with a comment '#' or newline */
%}
ALIASDECL ({LABEL}[ \t]*"="[ \t]*[^#\n\r]+)

%option yylineno

%%

{COM}        { /* skip comments */ }
^[ \t]*{ALIASDECL}/[#\n\r]  {
                /* Alias MUST end with a comment '#' or newline;
                   observe the trailing context*/
                yylval.str = strdup(yytext);
                return TOKEN_ALIAS;
              }
^[^=#\n\r]+/[#\n\r] {
      /* This pattern (line without alias declaration, no '=' sign)
         is for efficiency */
      yylval.str = strdup(yytext);
      return TOKEN_OTHER;
     }
.|\n {
      yylval.str = strdup(yytext);
      return TOKEN_OTHER;
     }
%%

/* In the event of some kind of error, this will get called. */
void yyerror(const char* msg)
{
    fprintf(stderr, "[aspp0] lexer error: %s in line %d\n", msg, yylineno);
    noerror = 0;
}

