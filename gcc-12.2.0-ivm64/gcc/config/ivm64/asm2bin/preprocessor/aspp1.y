%{
    /* IVM64 assembly preprocessor
       Alias preprocessing phase 1
       Author: Eladio Gutierrez
               University of Malaga, Spain
       Jan 2022
    */

    #include <stdio.h>
    int yylex(void);
    void yyerror(const char*);
    extern int noerror; /*Defined in .l*/

    #define YYERROR_VERBOSE 1
    extern int yylineno;

    /* Used when inserting a symbol in the symbol table */
    int redef;

    #define YYMAXDEPTH 1000000

    /* Symbol hash table size */
    // some primes: 5527 50227 502277
    #define IVM64_BISON_HASH_TABLE_SIZE 502277

    #define IVM64_PREPROCESSOR_1
    #include "ivm64_alias_helper.c"

    /* Where to write */
    FILE* out_file;

    /* Print redefined symbol error and exit*/
    static void print_error_redef(const char *s){
          fprintf(stderr, "[aspp1] Symbol '%s' already defined\n", s);
          fprintf(out_file, "[aspp1] Symbol '%s' already defined!!\n", s); /* This makes next stage fail */
          exit(EXIT_FAILURE);
    }

    struct alias_s {
        char *aliasfrom;
        char *aliasto;
    };

%}

/* These declare our output file names. */
/* This is not POSIX compliant */
//%output "y.tab.0.c"
//%defines "y.tab.0.h"

/* The field names of this union are used as return types for
   the tokens both in the lexer and the parser*/
%union {
    long int    num;
    char       *str;
}

/* Operands */
%token <str>TOKEN_ALIAS_DECL
%token <str>TOKEN_LABEL
%token <str>TOKEN_LABEL_DECL
%token <str>TOKEN_NUMBER
%token <str>TOKEN_OTHER
%token <str>TOKEN_WS      /* White space in alias section*/
%token <str>TOKEN_WS_CODE /* White space in code psection*/

%%

/* A valid assembly program is made of a
   sequence of valid statements */
assembler: prog  {/* Valid program */}
  ;

/* In this phase1, a valid program must be composed of:
     - An alias section with a sequence of alias declarations,
       followed by
     - A code section with not any alias
   Both parts are optional, but the alias section
   is always before the code section
*/
prog:
    /* empty */
  | alias_sequence
  | code_sequence
  | alias_sequence code_sequence
  ;

alias_sequence:
    alias_declaration
  | TOKEN_WS
  | alias_sequence TOKEN_WS
  | alias_sequence alias_declaration

alias_declaration:
    TOKEN_ALIAS_DECL {char *equal_sign = strstr($1, "=");
                     *equal_sign = 0; /* close the aliasfrom string*/
                     putsym_hash($1, equal_sign+1, &redef);
                     if (redef) {
                        print_error_redef($1);
                     }
                     free($1);
                    }
  ;


code_sequence:
    code_element
  | code_sequence code_element
  ;

code_element:
    TOKEN_LABEL {symrec *r = getsym_hash($1);
                 if (!r)
                     fprintf(out_file, "%s", $1);
                 else
                     fprintf(out_file, "%s", r->aliasto);
                 free($1);
                }
  | TOKEN_LABEL_DECL {
                       symrec *r = getsym_hash($1);
                       if (r) {
                            print_error_redef($1);
                       } else {
                            fprintf(out_file, "%s:", $1);
                       }
                       free($1);
                     }
  | TOKEN_NUMBER    {/* Add a blank to separate the number,
                        for example when processing
                        "X123=0x321 push!! 000X123" */
                     fprintf(out_file, "%s ", $1); free($1);}
  | TOKEN_OTHER     {fprintf(out_file, "%s", $1); free($1);}
  | TOKEN_WS_CODE   {fprintf(out_file, "%s", $1); free($1);}
  ;

%%
int main(int argc, char *argv[]){
    fprintf(stderr, "IVM64 flex/bison-based assembly preprocessor\n");
    out_file = stdout;
    init_symtable(IVM64_BISON_HASH_TABLE_SIZE);
    yyparse();
    exit(0);
}
