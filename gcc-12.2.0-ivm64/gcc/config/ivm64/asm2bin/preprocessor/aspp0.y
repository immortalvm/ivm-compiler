%{
    /* IVM64 assembly preprocessor
       Alias preprocessing phase 0
       Author: Eladio Gutierrez
               University of Malaga, Spain
       Jan 2022
    */

    #include <stdio.h>
    #include <locale.h>

    int yylex(void);
    void yyerror(const char*);
    extern int noerror; /*Defined in .l*/

    #define YYERROR_VERBOSE 1
    extern int yylineno;
    #define YYMAXDEPTH 1000000

    #define IVM64_PREPROCESSOR_0
    #include "ivm64_alias_helper.c"

    char *alias_filename;
    char *code_filename;

    FILE* alias_file;
    FILE* code_file;

    /* Where to write, in stdout or initialization file*/
    FILE* out_file;

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
%token <str>TOKEN_OTHER
%token <str>TOKEN_ALIAS

%%

/* A valid assembly program is made of a
   sequence of valid statements */
assembler: prog  {/* Valid program */}

prog:
    /* empty */
  | prog element
  ;

element:
    TOKEN_OTHER {fprintf(code_file, "%s", $1); free($1);}
  | TOKEN_ALIAS {
                  /* Parse a string like "a = b", adding to a list the pointers to "a" and "b" */
                  char *aliasfrom = NULL, *aliasto = NULL;
                  if (parse_alias($1, &aliasfrom, &aliasto)){
                      add_element(aliasfrom, aliasto); // This duplicates strings to be added to the list
                      if (aliasfrom) free(aliasfrom);
                      if (aliasto) free(aliasto);
                  }
                  free($1);
                }
  ;


%%
int main(int argc, char *argv[]){
    out_file = stdout;

    code_filename = gen_tempfile("_ivm64_bison_alias_code");
    code_file = fopen(code_filename, "w");

    setlocale(LC_ALL, ""); /* To support unicode, set system locale instead of default "C" */
    yyparse();   // This writes to code_file

    fclose(code_file);

    // Aliases are stored
    replace_alias_in_alias();
    alias_filename = gen_tempfile("_ivm64_bison_alias");
    dump_all_aliases_to_file(alias_filename);

    // Dump the aliases, then the code
    file_put_contents(alias_filename, out_file);
    file_put_contents(code_filename, out_file);

    remove(alias_filename);
    remove(code_filename);

    exit(0);
}
