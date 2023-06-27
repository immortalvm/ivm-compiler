/* IVM64 assembler
   Helper functions
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
#include <dirent.h>
#include <regex.h>

#include "../ivm64_helper.h"

#ifdef IVM64_PREPROCESSOR_0

// -------------------------------------------------------
/* Simple list to store aliases */
struct record
{
    char *aliasfrom;
    char *aliasto;
    struct record_t *next; /* link field */
};

typedef struct record record_t;
record_t* alias_list = (record_t*)0;

// add one element to list
record_t* add_element(char *aliasfrom, char *aliasto)
{
    record_t *p;
    p = (record_t *) malloc(sizeof(record_t));
    p->aliasfrom = strdup(aliasfrom);
    p->aliasto = strdup(aliasto);
    p->next = (struct record_t *)(alias_list);
    alias_list = p;
    return p;
}

void dump_all_aliases(FILE *fd)
{
    fprintf(fd, "# preprocessed alias start\n");
    for (record_t* p = alias_list ; p != (record_t*)0; p = (record_t*)p->next){
        /* For the next phase:
             - line must start with the aliasfrom
             - there must be NO spaces before the '=' sign */
        fprintf(fd, "%s=%s\n", p->aliasfrom, p->aliasto);
    }
    fprintf(fd, "# preprocessed alias end\n");
}

void dump_all_aliases_to_file(char *filename)
{
    FILE* fd = fopen(filename, "w"); /* This is our output */
    if (!fd) {
        fprintf(stderr, "Error opening symbol output file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }
    dump_all_aliases(fd);
    if (fd) fclose(fd);
}



/* Parse a string like "a = b", returning two allocated the pointers to "a" and "b".
   The global regexp_compiled is compiled in the main function for this regexp:
        char *alias_regexp="[ \t]*([^ \t]+)[ \t]*=[ \t]*([^ \t]+)";
        regcomp(&regexp_compiled, alias_regexp, REG_EXTENDED));
 */
int parse_alias(const char *S, char **aliasfrom, char **aliasto){
    /* Regexp for "label=alias", excluding leading blanks*/
    //alias_regexp="[ \t]*([A-Za-z_.][A-Za-z0-9_.]*)[ \t]*=[ \t]*([^ \t]|[^ \t].*[^ \t])";
    static char *alias_regexp="[ \t]*([^ \t]+)[ \t]*=[ \t]*(.+)";
    static regex_t regexp_compiled;
    static int compiled = 0; // Must be static
    if (!compiled) { // Compile if not yet compiled
        if (regcomp(&regexp_compiled, alias_regexp, REG_EXTENDED)) {
          fprintf(stderr, "[aspp0] Error compiling regexp for alias\n");
          exit(EXIT_FAILURE);
        }
        compiled = 1;
    }

    /* 3 groups */
    int maxng = 3;
    regmatch_t groups[maxng]; /*index 0=full matching, 1=1st group, 2=2nd group, ...*/
    if (regexec(&regexp_compiled, S, maxng, groups, 0) == 0) {
        char *dup = strdup(S);

        dup[groups[1].rm_eo] = 0;                   /*rm_eo=end index*/
        *aliasfrom = strdup(&dup[groups[1].rm_so]); /*rm_so=start index of the group*/

        dup[groups[2].rm_eo] = 0;
        *aliasto = strdup(&dup[groups[2].rm_so]);

        free(dup);

        //fprintf(stderr, "#%s #alias found\n", $1); //debug
        //fprintf(stderr, "%s = %s #alias processed \n", *aliasfrom, *aliasto); //debug

        // If the alias is an expression check that in the same line
        // parentheses are balanced
        char *s = *aliasto;
        int paropen=0, parclose=0;
        while (*s){
            if ('(' == *s) paropen++;
            if (')' == *s) parclose++;
            s++;
        }
        if (paropen != parclose) {
            fprintf(stderr, "[aspp0] Error: wrong expression when declaring alias '%s'\n", *aliasfrom);
            fprintf(stderr, "        Notice that multiline alias declaration is not supported\n");
            printf("[aspp0] Error:\n"); /* Make next stages fail */
            exit(EXIT_FAILURE);
        }

        return 1;
    } else {
        return 0;
    }
}



// -------------------------------------------------------
// Valid char to start a label
#define VALID_LABEL_UNICODE_P(c)   ((unsigned char)(c)<= 0xf4 && (unsigned char)(c)>= 0x80)
#define VALID_LABEL_START_CHAR_P(c)  ( ((c)>='A' && (c)<='Z') || ((c)>='a' && (c)<='z') || ((c)=='.') || ((c)=='_') || VALID_LABEL_UNICODE_P(c))
// Valid char in the middle of a label
#define VALID_LABEL_CHAR_P(c)  ( ((c)>='0' && (c)<='9') || VALID_LABEL_START_CHAR_P(c))

// Based on a string replacement routine from:
// https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c
char * label_replace(
    const char *original,
    const char *pattern,
    const char *replacement
) {
  size_t const orilen = strlen(original);
  size_t const patlen = strlen(pattern);
  size_t const replen = strlen(replacement);

  size_t patcnt = 0;
  const char * oriptr;
  const char * patloc;

  // find how many times the pattern occurs in the original string
  for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
  {
    patcnt++;
  }

  {
    // Use maxlen instead of replen because some replacements could not take place (a label as subpattern of another label)
    size_t const maxlen = (replen > patlen)?replen:patlen;
    // allocate memory for the new string
    //size_t const retlen = orilen + patcnt * (replen - patlen);
    size_t const retlen = orilen + patcnt * (maxlen - patlen);
    char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

    if (returned != NULL)
    {
      // copy the original string,
      // replacing all the instances of the pattern
      char * retptr = returned;
      for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
      {
        size_t const skplen = patloc - oriptr;
        // copy the section until the occurence of the pattern
        strncpy(retptr, oriptr, skplen);
        retptr += skplen;

        /* Do NOT replace if the substring is precededed or followed by a valid label char,
           because in this case it is not a label, but a part of a label*/
        const char *prevchar = retptr -1; /*Pointer to the char previous to the insertion */
        const char *nextchar = patloc + patlen; /*Pointer to the char after the pattern in the original
                                            string, that is, the next oriptr*/
        //fprintf(stderr, "pat='%s', repl='%15s' \t B: '%c' (%d), E: '%c'(%d)\n", pattern, replacement,
         //          *prevchar, VALID_LABEL_START_CHAR_P(*prevchar),
          //         *(nextchar), VALID_LABEL_START_CHAR_P(*(nextchar))); //debug
        if  ( ((retptr > returned) // not the start of the original string
               && VALID_LABEL_CHAR_P(*prevchar))
             || VALID_LABEL_CHAR_P(*(nextchar)) )
        {
            // copy the pattern (i.e. left untouched as it was)
            strncpy(retptr, pattern, patlen);
            retptr += patlen;
        } else {
            // copy the replacement
            strncpy(retptr, replacement, replen);
            retptr += replen;
        }
      }
      // copy the rest of the string.
      strcpy(retptr, oriptr);
    }
    return returned;
  }
}

/* Replace aliases in aliases
   Note that the substring replacement routine has been patched
   to replace substrings that are labels (that is separated) */
void replace_alias_in_alias(){
    int need_rerun;
    int max_rerun = 10;
    int max_len = 512*1024;
    do {
        need_rerun = 0;
        for (record_t* p = alias_list ; p != (record_t*)0; p = (record_t*)p->next){
            char* aliasfrom = p->aliasfrom;
            char* aliasto = p->aliasto;

            //fprintf(stderr, "REPLACE \n\t%s\n\t-> %s\n", aliasfrom, aliasto);  //debug

            for (record_t* q = (record_t*)alias_list ; q != (record_t*)0; q = (record_t*)q->next){
                //if (q==p) continue; // Do not replace itself
                // in p->aliasto replace q->aliasfrom with q->aliasto
                char *new_aliasto = label_replace(p->aliasto, q->aliasfrom, q->aliasto);
                if (strcmp(new_aliasto, p->aliasto)){
                    //fprintf(stderr, "\n\talias changed: old=%s new=%s\n", p->aliasto, new_aliasto);  //debug
                    need_rerun = 1; // An alias changed, re-run needed
                }
                free(p->aliasto);
                p->aliasto = new_aliasto;
                if (!strcmp(p->aliasfrom, p->aliasto) || strnlen(new_aliasto, max_len) >= max_len) {
                    fprintf(stderr, "Preprocessor error: potential alias cyclic definition (%s=...)\n", p->aliasfrom);
                    printf("[aspp0] Error\n"); /* Force next piped command fail */
                    exit(EXIT_FAILURE);
                }
            }
            //fprintf(stderr, "\n\tmax_rerun=%d\n", max_rerun);  //debug
        }
        max_rerun--; // Maximum number of re-runs
        if (!max_rerun) {
            fprintf(stderr, "Preprocessor error: maximum alias recursion reached\n");
            printf("[aspp0] Error\n"); /* Force next piped command fail */
            exit(EXIT_FAILURE);
        }
    } while (need_rerun && max_rerun > 0);
}

#endif


// -------------------------------------------------------

#ifdef IVM64_PREPROCESSOR_1

/* Hashed symbol table based on the simple list symbol table
   from: Compiler Construction using Flex and Bison, Anthony A. Aaby, 2003
   https://dlsiis.fi.upm.es/traductores/Software/Flex-Bison.pdf */
struct symrec
{
    char *aliasfrom;
    char *aliasto;
    struct symrec *next; /* link field */
};
typedef struct symrec symrec;

/* Symbol hash table, each entry is a pointer to
   a symbol linked list of symrec */
symrec** sym_table = NULL;
unsigned long int sym_table_size;

void init_symtable(unsigned long size){
   sym_table = (symrec**)malloc(size*sizeof(symrec*));
   for (long i=0; i<size; i++){
        sym_table[i] = (symrec*)0;
   }
   sym_table_size = size;
}

void destroy_symtable(){
    /* Deallocate all records in the list for each
       not empty table entry */
    /*TODO: implement this*/
}


/*getsym which returns a pointer to the symbol table entry corresponding
  to an identifier*/
symrec* getsym(char *aliasfrom, symrec* sym_table)
{
    symrec *ptr;
    for (ptr = sym_table; ptr != (symrec *)0; ptr = (symrec *)ptr->next)
        if (strcmp (ptr->aliasfrom, aliasfrom) == 0)
            return ptr;
    return 0;
}

/*putsym to put an identifier into the table*/
/* redefined can be 0 = new symbol
                    1 = symbol is redefined
*/
symrec* putsym(char *aliasfrom, char *aliasto, symrec** sym_table, int *redefined)
{
    symrec *ptr;
    if (ptr = getsym(aliasfrom, *sym_table)){
        /* Node exists*/
        *redefined = 1;
    } else {
        /* New node*/
        ptr = (symrec *) malloc (sizeof(symrec));
        ptr->aliasfrom = strdup(aliasfrom);
        ptr->aliasto = strdup(aliasto);
        ptr->next = (struct symrec *)(*sym_table);
        *sym_table = ptr;
        *redefined = 0;
    }
    return ptr;
}


// https://stackoverflow.com/questions/7666509/hash-function-for-string
// http://www.cse.yorku.ca/~oz/hash.html
// djb2 hash by Dan Bernstein
unsigned long hashfun(unsigned char *str, unsigned long maxval){
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % maxval;
}

/* Hashed versions of putsym, getsym */
symrec* putsym_hash(char *aliasfrom, char *aliasto, int *redefined){
    long hash = hashfun(aliasfrom, sym_table_size);
    return putsym(aliasfrom, aliasto, &sym_table[hash], redefined);
}

symrec* getsym_hash(char *aliasfrom){
    static int nrec = 0;
    long hash = hashfun(aliasfrom, sym_table_size);
    symrec* r = getsym(aliasfrom, sym_table[hash]);
    return r;
}
#endif


// COMMON CODE ========================================================

#undef IVM64_BISON_AS_0
#undef IVM64_BISON_AS_1
