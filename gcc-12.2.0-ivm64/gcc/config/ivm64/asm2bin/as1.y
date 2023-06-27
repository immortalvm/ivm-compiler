%{
    /* IVM64 assembler
       Parser phase 1
       Author: Eladio Gutierrez
               University of Malaga, Spain
       Jan 2022
    */

    /* This version optimizes the program size, based on
       the convergence of the size of push instructions.
       Also it tries to optimize jumps using jz when
       posible. */

    #include <stdio.h>
    int yylex(void);
    void yyerror(const char*);
    extern int noerror; /*Defined in .l*/
    extern void yyrestart(FILE*);

    #define YYERROR_VERBOSE 1
    extern int yylineno;

    /* Size of a push instruction */
    int push_size;

    /* Pass counter */
    int npass = 0;

    /* Maximum number of passes to converge */
    /* If defined IVM64_BISON_MAX_PASSES environment, use it */
    #define IVM64_BISON_MAX_PASSES (getenv("IVM64_BISON_MAX_PASSES")?atoi(getenv("IVM64_BISON_MAX_PASSES")):50)

    /* Symbol hash table size */
    // some primes: 5527 50227 502277
    #define IVM64_BISON_HASH_TABLE_SIZE 502277

    /* Disable optimizations if noopt==1 */
    int noopt = 0;

    /* Some insn counters to generate inner labels */
    long push_cnt = 0;
    long jz_cnt = 0;

    /* Used when inserting a symbol in the symbol table */
    int redef;

    /* Variables for debugging */
    int l_changed;
    int l_changed_p;
    int l_changed_n;
    long l_dec;

    /* There should not be GETPC instructions in native assembly,
       because optimizations can change the size of the binary code.
       If a GETPC was used explicitly in native asm to express
       addresses relative to PC, optimizations can make these offsets
       wrong. Use always labels to express memory addresses in native
       assembly, not PC (sugared assembler does not allow GETPC).
    */
    int getpc_found = 0;

    /* Name of the output files */
    #define IVM64_BISON_OUTPUT_BIN "z.bin"
    #define IVM64_BISON_OUTPUT_SYM "z.sym"

    /* A file where store stdin */
    #define IVM64_BISON_FILE_IN "_ivm64_bison_as1_input"
    char *input_file; /* Temporary input file to be generated later */

    /* Verbose info */
    #define IVM64_BISON_VERBOSE 0
    #define IVM64_BISON_VERBOSE_PASS_SIZE 0
    #define IVM64_BISON_VERBOSE_SUMMARY 0

    #define IVM64_BISON_AS_1
    #include "ivm64_helper_bison.c"

    /* Symbol table */
    static sym_table_t *T = NULL;

%}

/* These declare our output file names. */
/* This is not POSIX compliant */
//%output "y.tab.0.c"
//%defines "y.tab.0.h"

/* The field names of this union are used as return types for
   the tokens both in the lexer and the parser*/
%union {
    uint8_t    opcode;
    long       num;
    char       *label;
}

/* White spaces */
%token TOKEN_WS

/* Constant operands*/
%token <num>TOKEN_NUM
%token <label>TOKEN_LABEL
%token <label>TOKEN_NEAR_LABEL
%token <label>TOKEN_LABEL_DECL
%token <label>TOKEN_ALIAS_FROM

/* Native instructions */
%token <opcode>TOKEN_EXIT
%token <opcode>TOKEN_NOP
%token <opcode>TOKEN_JUMP
%token <opcode>TOKEN_JZFWD
%token <opcode>TOKEN_JZBACK
%token <opcode>TOKEN_SETSP
%token <opcode>TOKEN_GETPC
%token <opcode>TOKEN_GETSP
%token <opcode>TOKEN_PUSH0
%token <opcode>TOKEN_PUSH1
%token <opcode>TOKEN_PUSH2
%token <opcode>TOKEN_PUSH4
%token <opcode>TOKEN_PUSH8
%token <opcode>TOKEN_LOAD1
%token <opcode>TOKEN_LOAD2
%token <opcode>TOKEN_LOAD4
%token <opcode>TOKEN_LOAD8
%token <opcode>TOKEN_STORE1
%token <opcode>TOKEN_STORE2
%token <opcode>TOKEN_STORE4
%token <opcode>TOKEN_STORE8
%token <opcode>TOKEN_ADD
%token <opcode>TOKEN_MULT
%token <opcode>TOKEN_DIV
%token <opcode>TOKEN_REM
%token <opcode>TOKEN_LT
%token <opcode>TOKEN_AND
%token <opcode>TOKEN_OR
%token <opcode>TOKEN_NOT
%token <opcode>TOKEN_XOR
%token <opcode>TOKEN_POW
%token <opcode>TOKEN_CHECK
%token <opcode>TOKEN_PUTBYTE
%token <opcode>TOKEN_READCHAR
%token <opcode>TOKEN_PUTCHAR
%token <opcode>TOKEN_ADDSAMPLE
%token <opcode>TOKEN_SETPIXEL
%token <opcode>TOKEN_NEWFRAME
%token <opcode>TOKEN_READPIXEL
%token <opcode>TOKEN_READFRAME

/* Directives */
%token <num>TOKEN_DATA8
%token <num>TOKEN_DATA4
%token <num>TOKEN_DATA2
%token <num>TOKEN_DATA1

%token TOKEN_UNKNOWN

/* Type of non-terminal symbols */
%type <opcode>push0
%type <opcode>push_mnemonic
%type <opcode>incond_jump_insn
%type <opcode>other_insn

%type <num>data_start

%%

/* A valid assembly program is made of a
   sequence of valid statements */
assembler: prog   {/* Valid program */}

prog:
    /* empty */
  | prog statement
  ;

statement:
    label_declaration
  | native_insn
  | directive
  | alias
  ;

label_declaration:
    TOKEN_LABEL_DECL {
                       symrec* ps = getsym_hash(T, $1);
                       if (ps && ps->pc != Binprog.pos){
                            l_dec += (ps->pc - Binprog.pos);          /*Cumulative difference in label size*/
                            l_changed++;                              /*no. of labels that changed in this iteration*/
                            if (Binprog.pos <= ps->pc) l_changed_n++; /*labels that shrink or keep their size*/
                            if (Binprog.pos > ps->pc) l_changed_p++;  /*labels increasing in size*/
                       } //debug
                       putsym_hash(T, $1, Binprog.pos, NULL, 0, npass, &redef);
                       if (redef != 0 && redef != 1) {
                            fprintf(stderr, "[as1] Symbol '%s' already defined\n", $1);
                            exit(EXIT_FAILURE);
                       }
                       free($1);
                     }
  ;

native_insn:
    push_insn
  | cond_jump_insn
  | incond_jump_insn
  | other_insn       {write_bytes(1, $1);}
  ;

push_insn:
    push0 {write_bytes(1,$1);}
  | push_mnemonic TOKEN_NUM {
                            /* Ignoring push_size, and determining the size of the push
                               from the numeric constant */
                            int sz = long_size($2);     // size of NUM
                            int szn = long_size(~$2);   // size of negated NUM
                            if ( ($2) >= (1UL << 16) && IS_POWER_OF_TWO($2)){
                                /* If it is a big power of two, printing as a 2^n is smaller */
                                write_bytes(1, OPCODE_PUSH1);
                                write_bytes(1, LOG2($2));
                                write_bytes(1, OPCODE_POW);
                            /*TODO: if the negated is a power of two */
                            } else if (szn < sz){
                                // Use the negated if it is smaller
                                if (~($2)){
                                    if (szn == 1) write_bytes(1, OPCODE_PUSH1);
                                    if (szn == 2) write_bytes(1, OPCODE_PUSH2);
                                    if (szn == 4) write_bytes(1, OPCODE_PUSH4);
                                    if (szn == 8) write_bytes(1, OPCODE_PUSH8);
                                    write_bytes(szn, ~($2));
                                } else {
                                    write_bytes(1, OPCODE_PUSH0);
                                }
                                write_bytes(1, OPCODE_NOT);
                            } else {
                                // Default: push the constant according to its size
                                if ($2){
                                    if (sz == 1) write_bytes(1, OPCODE_PUSH1);
                                    if (sz == 2) write_bytes(1, OPCODE_PUSH2);
                                    if (sz == 4) write_bytes(1, OPCODE_PUSH4);
                                    if (sz == 8) write_bytes(1, OPCODE_PUSH8);
                                    write_bytes(sz, $2);
                                } else {
                                    write_bytes(1, OPCODE_PUSH0);
                                }
                            }
                         }
  | push_mnemonic TOKEN_LABEL {
                               int done = 0;
                               symrec* p = getsym_hash(T, $2);
                               if (!noopt && npass > 0) print_error_symbol_not_declared(p, $2); // When !noopt all symbols must be declared in the second pass

                               // True if the exact label value is known in this pass, not speculated from a previous pass
                               int backward = !(p && (p->pass < npass));

                               long adjust = 0;
                               /* By adjusting the value of future labels, the convergence speeds up drastrically */
                               /* With this aim, create an internal label for this push and compare the current PC with
                                  its previous value. This allows adjusting speculative (future) labels.*/
                               static char push_label[256];
                               sprintf(push_label, IVM64_BISON_TEMP_LABEL_PREFIX "_push_%ld", push_cnt++);
                               symrec* pl = getsym_hash(T, push_label);
                               if (pl && !backward){
                                     adjust = pl->pc - Binprog.pos ;
                                     //fprintf(stderr, "      [push] %s: pc=%ld, pl->pc=%ld, adjust=%ld\n", push_label, Binprog.pos, pl->pc, adjust); //debug
                                     //if (adjust < 0) {
                                     //    //fprintf(stderr, "adjust (%ld) < 0 in line %d\n", adjust, yylineno); //debug
                                     //    adjust = 0;
                                     //}
                               }
                               assert(adjust >= 0);
                               // Update push label with current pc
                               pl = putsym_hash(T, push_label, Binprog.pos, NULL, 0, npass, &redef);
                               pl->inner = 1;

                               if (p && p->absolute) {
                                   /* Numeric alias */
                                   //write_bytes(1, OPCODE_PUSH8); /* push8 */
                                   //write_bytes(8, p->pc);
                                   long val = p->pc;
                                   /* Select the smaller push instruction
                                      according to the value */
                                   if (val == 0){
                                       write_bytes(1, OPCODE_PUSH0);
                                   } else {
                                       /* Use the negated value if smaller*/
                                       int szp = long_size(val);
                                       int szn = long_size(~val);
                                       int sz = (szn<szp)?szn:szp;
                                       if (sz == 1) write_bytes(1, OPCODE_PUSH1);
                                       if (sz == 2) write_bytes(1, OPCODE_PUSH2);
                                       if (sz == 4) write_bytes(1, OPCODE_PUSH4);
                                       if (sz == 8) write_bytes(1, OPCODE_PUSH8);
                                       if (szn < szp){
                                           write_bytes(sz, ~val);
                                           write_bytes(1, OPCODE_NOT);
                                       } else {
                                           write_bytes(sz, val);
                                       }
                                   }
                               } else {
                                   /* standard label or label alias */
                                   long offset; /* This offset added to the current PC must produce the label value */

                                   if (p){
                                      /* Symbol already inserted in symbol table */
                                      offset = p->pc - Binprog.pos - 1;; /* label - PC + 1 */
                                      offset -= adjust; // Adjust offset for future labels (whose value is speculated from previous pass)
                                                        // This is crucial to accelerate convergence
                                   } else {
                                      /* This symbol reference is still unknown */
                                      offset = 0; /* A 0 is written in the program, until solving the pending references */
                                   }

                                   /* OPTIMIZATIONS FOR SIZE */
                                   /* IN THE FIRST PASS only labels already processed are known, and consequently its offset is
                                      negative.
                                      FOR SECOND AND SUCCESIVE PASSES labels processed have a true (negative) offset, but
                                      the rest of labels have a speculative (positive) offset based on previous pass
                                   */

                                   int szop = long_size(offset);
                                   int szon = long_size(~offset);
                                   int szo = (szon < szop)?szon:szop;

                                   if (!noopt) {
                                       /* Optimize small labels (<8)*/
                                       if (p
                                           && !done
                                           && (szo < 8)
                                           ){
                                            //fprintf(stderr, "small push label detected @pc=%ld offset_size=%d\n", Binprog.pos, long_size(-offset)); //debug
                                            write_bytes(1, OPCODE_GETPC); /* getpc */
                                            if (szo == 1 ) write_bytes(1, OPCODE_PUSH1); /* push1 */
                                            if (szo == 2 ) write_bytes(1, OPCODE_PUSH2); /* push2 */
                                            if (szo == 4 ) write_bytes(1, OPCODE_PUSH4); /* push4 */
                                            if (szon < szop){
                                                write_bytes(szo, ~offset);  /* the negated offset is smaller */
                                                write_bytes(1, OPCODE_NOT); /* not */
                                            } else {
                                                write_bytes(szo, offset);
                                            }
                                            write_bytes(1, OPCODE_ADD); /* add */
                                            done = 1;
                                       }
                                   }

                                   /* FALLBACK (NO OPTIMIZATION): use 8 bytes for the label.
                                      Used when no optimized push label is found
                                   */
                                   if (!done) {
                                       if (!p && noopt){
                                          /* This symbol reference is pending and we postpone to add its position*/
                                          long pos =  Binprog.pos + 2; /* The offset is stored in position current_pc+2 */
                                          offset =  - Binprog.pos - 1; /* 0 - PC + 1, offset to be added to the label */
                                          add_pending_ref($2, pos, offset, 8);
                                       }
                                       write_bytes(1, OPCODE_GETPC); /* getpc */
                                       write_bytes(1, OPCODE_PUSH8); /* push8 */
                                       write_bytes(8, offset);
                                       write_bytes(1, OPCODE_ADD);   /* add */
                                   }
                               }
                               free($2);
                              }
  | push_mnemonic TOKEN_LABEL '+' TOKEN_NUM {
                               // This is an optimization for "push! (+ label const)"
                               // Basically is the same as "push! label" above, but adding the constant value

                               long num = $4;

                               int done = 0;
                               symrec* p = getsym_hash(T, $2);
                               if (!noopt && npass > 0) print_error_symbol_not_declared(p, $2); // When !noopt all symbols must be declared in the second pass

                               // True if the exact label value is known in this pass, not speculated from a previous pass
                               int backward = !(p && (p->pass < npass));

                               long adjust = 0;
                               /* By adjusting the value of future labels, the convergence speeds up drastrically */
                               /* With this aim, create an internal label for this push and compare the current PC with
                                  its previous value. This allows adjusting speculative (future) labels.*/
                               static char push_label[256];
                               sprintf(push_label, IVM64_BISON_TEMP_LABEL_PREFIX "_push_%ld", push_cnt++);
                               symrec* pl = getsym_hash(T, push_label);
                               if (pl && !backward){
                                     adjust = pl->pc - Binprog.pos ;
                                     //fprintf(stderr, "      [push] %s: pc=%ld, pl->pc=%ld, adjust=%ld\n", push_label, Binprog.pos, pl->pc, adjust); //debug
                                     //if (adjust < 0) {
                                     //    //fprintf(stderr, "adjust (%ld) < 0 in line %d\n", adjust, yylineno); //debug
                                     //    adjust = 0;
                                     //}
                               }
                               assert(adjust >= 0);
                               // Update push label with current pc
                               pl = putsym_hash(T, push_label, Binprog.pos, NULL, 0, npass, &redef);
                               pl->inner = 1;

                               if (p && p->absolute) {
                                   /* Numeric alias */
                                   //write_bytes(1, OPCODE_PUSH8); /* push8 */
                                   //write_bytes(8, p->pc);
                                   long val = p->pc + num;
                                   /* Select the smaller push instruction
                                      according to the value */
                                   if (val == 0){
                                       write_bytes(1, OPCODE_PUSH0);
                                   } else {
                                       /* Use the negated value if smaller*/
                                       int szp = long_size(val);
                                       int szn = long_size(~val);
                                       int sz = (szn<szp)?szn:szp;
                                       if (sz == 1) write_bytes(1, OPCODE_PUSH1);
                                       if (sz == 2) write_bytes(1, OPCODE_PUSH2);
                                       if (sz == 4) write_bytes(1, OPCODE_PUSH4);
                                       if (sz == 8) write_bytes(1, OPCODE_PUSH8);
                                       if (szn < szp){
                                           write_bytes(sz, ~val);
                                           write_bytes(1, OPCODE_NOT);
                                       } else {
                                           write_bytes(sz, val);
                                       }
                                   }
                               } else {
                                   /* standard label or label alias */
                                   long offset; /* This offset added to the current PC must produce the label value */

                                   if (p){
                                      /* Symbol already inserted in symbol table */
                                      offset = p->pc - Binprog.pos - 1;; /* label - PC + 1 */
                                      offset -= adjust; // Adjust offset for future labels (whose value is speculated from previous pass)
                                                        // This is crucial to accelerate convergence
                                   } else {
                                      /* This symbol reference is still unknown */
                                      offset = 0; /* A 0 is written in the program, until solving the pending references */
                                   }

                                   offset += num;

                                   /* OPTIMIZATIONS FOR SIZE */
                                   /* IN THE FIRST PASS only labels already processed are known, and consequently its offset is
                                      negative.
                                      FOR SECOND AND SUCCESIVE PASSES labels processed have a true (negative) offset, but
                                      the rest of labels have a speculative (positive) offset based on previous pass
                                   */

                                   int szop = long_size(offset);
                                   int szon = long_size(~offset);
                                   int szo = (szon < szop)?szon:szop;

                                   if (!noopt) {
                                       /* Optimize small labels (<8)*/
                                       if (p
                                           && !done
                                           && (szo < 8)
                                           ){
                                            //fprintf(stderr, "small push label detected @pc=%ld offset_size=%d\n", Binprog.pos, long_size(-offset)); //debug
                                            write_bytes(1, OPCODE_GETPC); /* getpc */
                                            if (szo == 1 ) write_bytes(1, OPCODE_PUSH1); /* push1 */
                                            if (szo == 2 ) write_bytes(1, OPCODE_PUSH2); /* push2 */
                                            if (szo == 4 ) write_bytes(1, OPCODE_PUSH4); /* push4 */
                                            if (szon < szop){
                                                write_bytes(szo, ~offset);  /* the negated offset is smaller */
                                                write_bytes(1, OPCODE_NOT); /* not */
                                            } else {
                                                write_bytes(szo, offset);
                                            }
                                            write_bytes(1, OPCODE_ADD); /* add */
                                            done = 1;
                                       }
                                   }

                                   /* FALLBACK (NO OPTIMIZATION): use 8 bytes for the label.
                                      Used when no optimized push label is found
                                   */
                                   if (!done) {
                                       if (!p && noopt){
                                          /* This symbol reference is pending and we postpone to add its position*/
                                          long pos =  Binprog.pos + 2; /* The offset is stored in position current_pc+2 */
                                          offset =  - Binprog.pos - 1 + num; /* 0 - PC + 1 + num, offset to be added to the label */
                                          add_pending_ref($2, pos, offset, 8);
                                       }
                                       write_bytes(1, OPCODE_GETPC); /* getpc */
                                       write_bytes(1, OPCODE_PUSH8); /* push8 */
                                       write_bytes(8, offset);
                                       write_bytes(1, OPCODE_ADD);   /* add */
                                   }
                               }
                               free($2);
                              }
  | push_mnemonic TOKEN_NEAR_LABEL {
                               symrec* p = getsym_hash(T, $2);
                               if (!noopt && npass > 0) print_error_symbol_not_declared(p, $2); // When !noopt all symbols must be declared in the second pass

                               // True if the exact label value is known in this pass, not speculated from a previous pass
                               int backward = !(p && (p->pass < npass));

                               long adjust = 0; /* Adjust is not necessary as this prints a fixed size instruction */

                               if (p && p->absolute) {
                                   /* Numeric alias */
                                   assert(p->pc >= 0 && p->pc <= 0xff);
                                   write_bytes(1, OPCODE_PUSH1); /* near labels are 1-byte wide */
                                   write_bytes(1, p->pc);
                               } else {
                                   /* standard label or label alias */
                                   long offset; /* This offset added to the current PC must produce the label value */

                                   if (p){
                                      /* Symbol already inserted in symbol table */
                                      offset = p->pc - Binprog.pos - 1;; /* label - PC + 1 */
                                      offset -= adjust; // Adjust offset for future labels (whose value is speculated from previous pass)
                                                        // This is crucial to accelerate convergence
                                   } else {
                                      /* This symbol reference is pending and we postpone to add its position*/
                                      if (noopt){
                                         long pos =  Binprog.pos + 2; /* The offset is stored in position current_pc+2 */
                                         offset =  - Binprog.pos - 1; /* 0 - PC + 1, offset to be added to the label */
                                         add_pending_ref($2, pos, offset, 1);
                                      }
                                      offset = 0; /* A 0 is written in the program, until solving the pending references */
                                   }

                                   /* near labels has always 1 byte */
                                   write_bytes(1, OPCODE_GETPC); /* getpc */
                                   write_bytes(1, OPCODE_PUSH1); /* push1 */
                                   write_bytes(1, offset);
                                   write_bytes(1, OPCODE_ADD);   /* add */
                               }
                               free($2);
                              }
  ;

push0:
    TOKEN_PUSH0
  ;

push_mnemonic:
    TOKEN_PUSH1 {push_size = 1;}
  | TOKEN_PUSH2 {push_size = 2;}
  | TOKEN_PUSH4 {push_size = 4;}
  | TOKEN_PUSH8 {push_size = 8;}
  ;

/* The operand of conditional instructions is always 1-byte wide */
cond_jump_insn:
    TOKEN_JZFWD TOKEN_NUM   {write_bytes(1, $1); write_bytes(1, $2);}
  | TOKEN_JZBACK TOKEN_NUM  {write_bytes(1, $1); write_bytes(1, $2);}
  | jz_to_label_insn
  ;

/* "jzfwd label" is an optimized form that can be
   written in different ways depending on the label:
   "jzfwd N" "jzback N" or "push label jmp".
   Observe that although mnemonic jzfwd is used, it can be
   a backward jump */
jz_to_label_insn:
    TOKEN_JZFWD TOKEN_LABEL  {
                                /* implement jz to label*/
                                symrec* p = getsym_hash(T, $2);
                                if (!noopt && npass > 0) print_error_symbol_not_declared(p, $2); // When !noopt all symbols must be declared in the second pass

                                // True if the exact label value is known in this pass, not speculated from a previous pass
                                int backward = !(p && (p->pass < npass));

                                int done = 0;

                                long adjust = 0;
                                /* By adjusting the value of future labels, the convergence speeds up drastrically */
                                /* With this aim, create an internal label for this push and compare the current PC with
                                   its previous value. This allows adjusting speculative (future) labels.*/
                                static char jz_label[256]; /* A inner label associated to each jz */
                                sprintf(jz_label, IVM64_BISON_TEMP_LABEL_PREFIX "_jz_%ld", jz_cnt++);
                                symrec* pl = getsym_hash(T, jz_label);
                                if (pl && !backward){
                                      adjust = pl->pc - Binprog.pos ;
                                      //fprintf(stderr, "      [push] %s: pc=%ld, pl->pc=%ld, adjust=%ld\n", jz_label, Binprog.pos, pl->pc, adjust); //debug
                                      //if (adjust < 0) {
                                      //    //fprintf(stderr, "adjust (%ld) < 0 in line %d\n", adjust, yylineno); //debug
                                      //    adjust = 0;
                                      //}
                                }
                                assert(adjust >= 0);
                                // Update push label with current pc
                                pl = putsym_hash(T, jz_label, Binprog.pos, NULL, 0, npass, &redef);
                                pl->inner = 1;

                                if (p && p->absolute) {
                                   /* Numeric alias */
                                   assert( p->pc >= 0 && p->pc <= 0xff);
                                   write_bytes(1, OPCODE_JZFWD);
                                   write_bytes(1, p->pc);
                                } else {
                                   /* standard label or label alias */
                                   long offset; /* This offset added to the current PC must produce the label value */

                                   if (p){
                                      /* Symbol already inserted in symbol table */
                                      offset = p->pc - Binprog.pos - 1;; /* label - PC + 1 */
                                      offset -= adjust; // Adjust offset for future labels (whose value is speculated from previous pass)
                                                        // This is crucial to accelerate convergence
                                   } else {
                                      /* This symbol reference is still unknown */
                                      offset = 0; /* A 0 is written in the program, until solving the pending references */
                                   }

                                   /* Optimized jz to label */
                                   if (!noopt){
                                        /* If the offset fill in one byte, then:
                                              "jzfwd or "jzback" is used.
                                           Otherwise "jump_zero! label" is expanded according to the label
                                           size, N, as:
                                                jzfwd 3
                                                push0 jzfwd <N+3>
                                                push<N> <label>  #push label = getpc + push<N> offset + add = N+3 bytes
                                                jump
                                        */
                                        if (p){ /*Known label, or speculative*/
                                            if (backward) { /* Known label */
                                                /* 1-byte offset*/
                                                if (!done && -offset <= 255 && -offset >= 0){
                                                    /* back jump with 1-byte offset */
                                                    write_bytes(1, OPCODE_JZBACK);
                                                    write_bytes(1, -offset);
                                                    done = 1;
                                                }

                                                /* 1,2 or 4-byte offset*/
                                                /* As jumping backward, the offset must be negative, so
                                                   the negated offset is tried */
                                                int offsetfix = 5; /* Cumulative offset previous to the getpc asociated to "push8 label"*/
                                                int szo = long_size(~(offset -offsetfix));
                                                if (!done && szo < 8) {
                                                    write_bytes(1, OPCODE_JZFWD);
                                                    write_bytes(1, 3);
                                                    write_bytes(1, OPCODE_PUSH0);
                                                    write_bytes(1, OPCODE_JZFWD);
                                                    write_bytes(1, szo + 5);

                                                    write_bytes(1, OPCODE_GETPC);
                                                    if (szo == 1 ) write_bytes(1, OPCODE_PUSH1); /* push1 */
                                                    if (szo == 2 ) write_bytes(1, OPCODE_PUSH2); /* push2 */
                                                    if (szo == 4 ) write_bytes(1, OPCODE_PUSH4); /* push4 */
                                                    write_bytes(szo, ~(offset - offsetfix));
                                                    write_bytes(1, OPCODE_NOT); /* not */
                                                    write_bytes(1, OPCODE_ADD);
                                                    write_bytes(1, OPCODE_JUMP);

                                                    done = 1;
                                                }
                                            } else { /* jumping forward, thus, to a speculatively known label */
                                                /* 1-byte offset*/
                                                if ( !done && offset >= 1 && offset <= 255 ){
                                                    write_bytes(1, OPCODE_JZFWD);      /* jzfwd */
                                                    write_bytes(1, offset-1);  /* offset forward (must fill in one byte)*/
                                                    done = 1;
                                                }

                                                /* 1,2 or 4-byte offset*/
                                                /* As jumping forward, the offset must be positive */
                                                int offsetfix = 5; /* Cumulative offset previous to the getpc asociated to "push8 label"*/
                                                int szo = long_size((offset -offsetfix));
                                                if (!done && szo < 8) {

                                                    write_bytes(1, OPCODE_JZFWD);
                                                    write_bytes(1, 3);
                                                    write_bytes(1, OPCODE_PUSH0);
                                                    write_bytes(1, OPCODE_JZFWD);
                                                    write_bytes(1, szo + 4);

                                                    write_bytes(1, OPCODE_GETPC);
                                                    if (szo == 1 ) write_bytes(1, OPCODE_PUSH1); /* push1 */
                                                    if (szo == 2 ) write_bytes(1, OPCODE_PUSH2); /* push2 */
                                                    if (szo == 4 ) write_bytes(1, OPCODE_PUSH4); /* push4 */
                                                    write_bytes(szo, offset - offsetfix);
                                                    write_bytes(1, OPCODE_ADD);
                                                    write_bytes(1, OPCODE_JUMP);

                                                    done = 1;
                                                }

                                            }
                                        }
                                   }

                                   /* FALLBACK (NO OPTIMIZATION): use 8 bytes for the label */
                                   if (!done) {
                                        /* A "jump_zero! label" was expanded as:
                                            jzfwd 3
                                            push0 jzfwd 12
                                            #using push1 as a mark for this case (although it's push8 by default)
                                            push8 <label>  #push label = getpc + push8 offset + add = 11 bytes
                                            jump
                                        */
                                        long offsetfix = 5; /* Cumulative offset previous to the getpc asociated to "push8 label"*/
                                        write_bytes(1, OPCODE_JZFWD);
                                        write_bytes(1, 3);
                                        write_bytes(1, OPCODE_PUSH0);
                                        write_bytes(1, OPCODE_JZFWD);
                                        write_bytes(1, 12); // offsetfix = 5
                                        if (!p && noopt){
                                             /* When optimization is disable, this symbol reference is pending
                                                and we postpone to add its position*/
                                             long pos =  Binprog.pos + 2; /* The offset is stored in position current_pc+2 */
                                             long poffset =  - Binprog.pos - 1; /* 0 - PC + 1, offset to be added to the label */
                                             add_pending_ref($2, pos, poffset, 8);
                                        }
                                        write_bytes(1, OPCODE_GETPC);
                                        write_bytes(1, OPCODE_PUSH8);
                                        write_bytes(8, offset - offsetfix);
                                        write_bytes(1, OPCODE_ADD);
                                        write_bytes(1, OPCODE_JUMP);
                                   }

                                }
                                free($2);
                             }
  |  TOKEN_JZFWD TOKEN_NEAR_LABEL  {
                                /* implement jzfwd to a short label (known to be near)*/
                                symrec* p = getsym_hash(T, $2);
                                if (!noopt && npass > 0) print_error_symbol_not_declared(p, $2); // When !noopt all symbols must be declared in the second pass

                                long adjust = 0; /* Adjust is not necessary as this prints a fixed size instruction */

                                if (p && p->absolute) {
                                   /* Numeric alias */
                                   assert(p->pc >= 0 && p->pc <= 0xff);
                                   write_bytes(1, OPCODE_JZFWD);
                                   write_bytes(1, p->pc);
                                } else {
                                   /* standard label or label alias */
                                   long offset; /* This offset added to the current PC must produce the label value */

                                   if (p){
                                      /* Symbol already inserted in symbol table */
                                      offset = p->pc - Binprog.pos - 1;; /* label - PC + 1 */
                                      offset -= adjust; // Adjust offset for future labels (whose value is speculated from previous pass)
                                                        // This is crucial to accelerate convergence
                                   } else {
                                      /* This symbol reference is pending and we postpone to add its position*/
                                      if (noopt){
                                           /* Whe optimization is disabled, this symbol reference is pending
                                              and we postpone to add its position*/
                                           long pos =  Binprog.pos + 1; /* The offset is stored in position current_pc+2 */
                                           long poffset =  - Binprog.pos -2; /* 0 - PC + 1, offset to be added to the label */
                                           add_pending_ref($2, pos, poffset, 1);
                                      }
                                      offset = 0; /* A 0 is written in the program, until solving the pending references */
                                   }

                                   /* 1-byte offset*/
                                   write_bytes(1, OPCODE_JZFWD);
                                   write_bytes(1, offset-1);
                                }
                                free($2);
                             }
  ;

incond_jump_insn:
    TOKEN_JUMP { /* simple case */
                 write_bytes(1, $1);
               }
  | TOKEN_JUMP TOKEN_LABEL{
                           /* implement jump to label*/
                           symrec* p = getsym_hash(T, $2);
                           if (!noopt && npass > 0) print_error_symbol_not_declared(p, $2); // When !noopt all symbols must be declared in the second pass

                           // True if the exact label value is known in this pass, not speculated from a previous pass
                           int backward = !(p && (p->pass < npass));

                           int done = 0;

                           long adjust = 0;
                           /* By adjusting the value of future labels, the convergence speeds up drastrically */
                           /* With this aim, create an internal label for this push and compare the current PC with
                              its previous value. This allows adjusting speculative (future) labels.*/
                           static char jump_label[256]; /* A inner label associated to each jz */
                           sprintf(jump_label, IVM64_BISON_TEMP_LABEL_PREFIX "_jump_%ld", jz_cnt++);
                           symrec* pl = getsym_hash(T, jump_label);
                           if (pl && !backward){
                                 adjust = pl->pc - Binprog.pos ;
                                 //fprintf(stderr, "      [push] %s: pc=%ld, pl->pc=%ld, adjust=%ld\n", jump_label, Binprog.pos, pl->pc, adjust); //debug
                                 //if (adjust < 0) {
                                 //    //fprintf(stderr, "adjust (%ld) < 0 in line %d\n", adjust, yylineno); //debug
                                 //    adjust = 0;
                                 //}
                           }
                           assert(adjust >= 0);
                           // Update push label with current pc
                           pl = putsym_hash(T, jump_label, Binprog.pos, NULL, 0, npass, &redef);
                           pl->inner = 1;

                           if (p && p->absolute) {
                              /* Numeric alias */
                              write_bytes(1, OPCODE_PUSH8);
                              write_bytes(8, p->pc);
                              write_bytes(1, OPCODE_JUMP);
                           } else {
                              /* standard label or label alias */
                              long offset; /* This offset added to the current PC must produce the label value */

                              if (p){
                                 /* Symbol already inserted in symbol table */
                                 offset = p->pc - Binprog.pos - 1;; /* label - PC + 1 */
                                 offset -= adjust; // Adjust offset for future labels (whose value is speculated from previous pass)
                                                   // This is crucial to accelerate convergence
                              } else {
                                 if (noopt){
                                      /* Whe optimization is disabled, this symbol reference is pending
                                         and we postpone to add its position*/
                                      long pos =  Binprog.pos + 2; /* The offset is stored in position current_pc+2 */
                                      long poffset =  - Binprog.pos - 1; /* 0 - PC + 1, offset to be added to the label */
                                      add_pending_ref($2, pos, poffset, 8);
                                 }
                                 offset = 0; /* A 0 is written in the program, until solving the pending references */
                              }

                              /* Optimized jump to label */
                              if (!noopt){
                                   /* If the offset fill in one byte, then:
                                         "push0 jzfwd N" or "push0 jzback N" is used.
                                      Otherwise "jump! label" is expanded according to the label
                                      size, N, as "push! label jump".
                                   */
                                   if (p){ /*Known label, or speculative*/
                                       if (backward) { /* Known label */
                                           /* 1-byte offset*/
                                           int offsetfix=1; /*Fixing offset on reducing the size */
                                           if ( !done && -offset + offsetfix <= 255 && -offset + offsetfix >= 0){
                                               /* back jump with 1-byte offset */
                                               write_bytes(1, OPCODE_PUSH0);
                                               write_bytes(1, OPCODE_JZBACK);
                                               write_bytes(1, -offset + offsetfix);
                                               done = 1;
                                           }

                                           /* 1,2 or 4-byte offset*/
                                           /* As jumping backward, the offset must be negative, so
                                              the negated offset is tried */
                                           int szo = long_size(~offset);
                                           if (!done && szo < 8) {
                                               write_bytes(1, OPCODE_GETPC);
                                               if (szo == 1 ) write_bytes(1, OPCODE_PUSH1); /* push1 */
                                               if (szo == 2 ) write_bytes(1, OPCODE_PUSH2); /* push2 */
                                               if (szo == 4 ) write_bytes(1, OPCODE_PUSH4); /* push4 */
                                               write_bytes(szo, ~offset);
                                               write_bytes(1, OPCODE_NOT); /* not */
                                               write_bytes(1, OPCODE_ADD);
                                               write_bytes(1, OPCODE_JUMP);

                                               done = 1;
                                           }
                                       } else { /* jumping forward, thus, to a speculatively known label */
                                           /* 1-byte offset*/
                                           int offsetfix = -2; /*Fixing offset on reducing the size */
                                           if (!done && offset + offsetfix >= 0 && offset + offsetfix <= 255 ){
                                               write_bytes(1, OPCODE_PUSH0);
                                               write_bytes(1, OPCODE_JZFWD);
                                               write_bytes(1, offset + offsetfix);  /* offset forward (must fill in one byte)*/
                                               done = 1;
                                           }

                                           /* 1,2 or 4-byte offset*/
                                           /* As jumping forward, the offset must be positive */
                                           offsetfix = 0; /* Cumulative offset previous to the getpc asociated to "push8 label"*/
                                           int szo = long_size(offset -offsetfix);
                                           if (!done && szo < 8) {
                                               write_bytes(1, OPCODE_GETPC);
                                               if (szo == 1 ) write_bytes(1, OPCODE_PUSH1); /* push1 */
                                               if (szo == 2 ) write_bytes(1, OPCODE_PUSH2); /* push2 */
                                               if (szo == 4 ) write_bytes(1, OPCODE_PUSH4); /* push4 */
                                               write_bytes(szo, offset - offsetfix);
                                               write_bytes(1, OPCODE_ADD);
                                               write_bytes(1, OPCODE_JUMP);
                                               done = 1;
                                           }

                                       }
                                   }
                              }

                              /* FALLBACK (NO OPTIMIZATION): use 8 bytes for the label */
                              if (!done) {
                                   /* A "jump! label" was expanded as:
                                       push8 <label>  #push label = getpc + push8 offset + add = 11 bytes
                                       jump
                                   */
                                   write_bytes(1, OPCODE_GETPC);
                                   write_bytes(1, OPCODE_PUSH8);
                                   write_bytes(8, offset);
                                   write_bytes(1, OPCODE_ADD);
                                   write_bytes(1, OPCODE_JUMP);
                              }

                           }
                           free($2);
                          }
  ;

other_insn:
    TOKEN_EXIT
  | TOKEN_NOP
  | TOKEN_SETSP
  | TOKEN_GETPC { if (!getpc_found){
                    getpc_found=1;
                    fprintf(stderr, "[as1] Warning: GETPC found in native assembly!: "
                    "optimizations can generate bad offsets;\n"
                    "      Use always labels to reference program addresses.\n");
                  }
                }
  | TOKEN_GETSP
  | TOKEN_LOAD1
  | TOKEN_LOAD2
  | TOKEN_LOAD4
  | TOKEN_LOAD8
  | TOKEN_STORE1
  | TOKEN_STORE2
  | TOKEN_STORE4
  | TOKEN_STORE8
  | TOKEN_ADD
  | TOKEN_MULT
  | TOKEN_DIV
  | TOKEN_REM
  | TOKEN_LT
  | TOKEN_AND
  | TOKEN_OR
  | TOKEN_NOT
  | TOKEN_XOR
  | TOKEN_POW
  | TOKEN_CHECK
  | TOKEN_PUTBYTE
  | TOKEN_READCHAR
  | TOKEN_PUTCHAR
  | TOKEN_ADDSAMPLE
  | TOKEN_SETPIXEL
  | TOKEN_NEWFRAME
  | TOKEN_READPIXEL
  | TOKEN_READFRAME
  ;


directive:
    data
  ;

data:
    data_start '[' TOKEN_NUM  ']' {write_bytes($1, $3);}
  ;

data_start:
    TOKEN_DATA8
  | TOKEN_DATA4
  | TOKEN_DATA2
  | TOKEN_DATA1
  ;

alias:
    TOKEN_ALIAS_FROM  TOKEN_LABEL { putsym_hash(T, $1, 0, $2, 0, npass, &redef);
                                    if (redef != 0 && redef != 1) {
                                        fprintf(stderr, "[as1] Symbol '%s' already defined\n", $1);
                                        exit(EXIT_FAILURE);
                                    }
                                    free($1);
                                    free($2);
                                  }
  | TOKEN_ALIAS_FROM  TOKEN_NUM   { putsym_hash(T, $1, $2, NULL, 1, npass, &redef);
                                    if (redef != 0 && redef != 1) {
                                        fprintf(stderr, "[as1] Symbol '%s' already defined\n", $1);
                                        exit(EXIT_FAILURE);
                                    }
                                    free($1);
                                  }

%%

/* Remove temporary input file, to be used with atexit() */
void remove_files(void){
    if (input_file){
        remove(input_file);
    }
}


int main(int argc, char *argv[]){

    /* Options:
         -b binary output file name
         -s symbol output file name
         -noopt do not optimize size
    */
    int c;
    char *bin_filename = IVM64_BISON_OUTPUT_BIN;
    char *sym_filename = IVM64_BISON_OUTPUT_SYM;
    noopt = 0;
    while ((c = getopt (argc, argv, "b:s:n:")) != -1) {
        switch (c) {
          case 'b':
            if (optarg) {
                bin_filename = strdup(optarg);
            }
            break;
          case 's':
            if (optarg) {
                sym_filename = strdup(optarg);
            }
            break;
          case 'n':
            if (optarg && !strcmp("oopt", optarg)) {
                noopt = 1;
            }
            break;
          default:
            fprintf(stderr, "Usage: %s [-b bin_file] [-s symbol_file] [-noopt]\n", argv[0]);
            exit(EXIT_FAILURE);
         }
    }

    init_bin(10000000);
    T = init_symtable(IVM64_BISON_HASH_TABLE_SIZE);

    int r=0;
    if (!noopt) {
        /* MULTIPASS PARSING to optimize program size */

        /* Dump stdin to file to do multiple passes */
        input_file = gen_tempfile(IVM64_BISON_FILE_IN);
        stdin_to_file(input_file);

        atexit(remove_files);

        FILE *fd;
        long last_prog_size=1UL<<62, prev_prog_size=1UL<<62;

        /* Do until converging */
        do{
            l_changed = 0;
            l_changed_n = 0;
            l_changed_p = 0;
            l_dec=0;

            push_cnt = 0;
            jz_cnt = 0;

            Binprog.pos = 0; /* restart PC*/
            fd = fopen(input_file, "r");
            yyrestart(fd);   /* Do not forget that this DOES NOT initialize flex start conditions */
            yylineno = 0;
            r = yyparse();
            fclose(fd);
            prev_prog_size = last_prog_size;
            last_prog_size = Binprog.pos;
            #if IVM64_BISON_VERBOSE_PASS_SIZE == 1
                fprintf(stderr, "   pass[%d] bytes=%ld (%+ld)  \t| no. labels w/ change=%d (-:%d / +:%d) \tavg. label decreasing=%.1f\n",
                                     npass, last_prog_size, (npass>0)?(last_prog_size-prev_prog_size):last_prog_size,
                                     l_changed, l_changed_n, l_changed_p, (l_changed>0)?(l_dec*1.0/l_changed):0); //debug
            #else
                fprintf(stderr, ".");
                if (npass>0 && last_prog_size == prev_prog_size) fprintf(stderr, "\n");
            #endif
            //
            //char sym_filename_pass[2048];
            //sprintf(sym_filename_pass, "zpass%d.sym", npass);
            //dump_sym_file(sym_filename_pass); // debug symbols in each pass

            npass++;

            /* If the maximum number of passes is exceeded without
               convergence, force a not optimized single pass */
            if (npass > IVM64_BISON_MAX_PASSES){
                fprintf(stderr, "Number of passes to converge (%d) exceeded: re-running parsing w/o optimization\n", IVM64_BISON_MAX_PASSES);
                noopt = 1; /* Re-run parsing w/o optimization */
                destroy_symtable(&T);
                T = init_symtable(IVM64_BISON_HASH_TABLE_SIZE);
                free(Binprog.content);
                init_bin(10000000);
                fd = fopen(input_file, "r");
                yyrestart(fd);
                yylineno = 0;
                r = yyparse();
                fclose(fd);
                solve_pending_references(T);
                break;
            }
        } while (prev_prog_size != last_prog_size || l_changed);

        remove(input_file);
    } else {
        /* SINGLE PASS (noopt==1, no size optimization) */
        r = yyparse(); /* Read from stdin*/
        solve_pending_references(T);
    }

    if (! noerror){ /* Error in lexer */
        fprintf(stderr, "Error: a lexer error was found ... as1 exiting\n");
        exit(EXIT_FAILURE);
    }

    long nsym = print_symtable(T, IVM64_BISON_VERBOSE); //debug

    /* Dump the binary (.bin) and the symbol file (.sym)*/
    long s = dump_bin(bin_filename);
    dump_sym_file(T, sym_filename);

    #if IVM64_BISON_VERBOSE_SUMMARY == 1
    fprintf(stderr, "Total symbols=%ld\n", nsym);
    fprintf(stderr, "Program size %ld bytes\n", s);
    #endif

    return r;
}
