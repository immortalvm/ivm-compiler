/* IVM64 assembler
   Helper functions
   Author: Eladio Gutierrez
           University of Malaga, Spain
   Jan 2022 - Jun 2023
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <assert.h>
#include "ivm64_helper.h"

#define BYTES_PER_WORD 8

#define IVM_BINARY_VERSION 2

// COMMON CODE ========================================================
int long_size(uint64_t x){
    if (x <= 0xffUL) return 1;
    if (x <= 0xffffUL) return 2;
    if (x <= 0xffffffffUL) return 4;
    return 8;
}

#define IS_POWER_OF_TWO(N) ((N) && (!((N) & ((N)-1))))
#define LOG2(X) ((unsigned) (8*sizeof (unsigned long) - __builtin_clzll((X)) - 1))

#define IVM64_BISON_TEMP_LABEL_PREFIX ".LIVM64_bison_expand_"

// CODE FOR PHASE 0 ASSEMBLER ========================================================
#ifdef IVM64_BISON_AS_0

void start_complex_data();
void stop_complex_data();
void print_neg();
void print_not();

void print_token_num(long num){
    if (!indata || gen_init_code) {
        if (num){
            // Optimizing push size is done in as1 stage
            fprintf(out_file, "push%d %ld\n", long_size(num), num);
        } else {
            fprintf(out_file, "push0\n");
        }
    } else {
        fprintf(out_file, "data%d [%ld]\n", indata, num);
    }
}

/* Check if a label is a mnemonic, to be marked in the phase0 for phase1*/
/* Note that gramatically mnemonics in each phase are not exactly the same */
int label_is_mnemonic(char *l){
    char *mnemonics[] = {
        /*mnemonics in phase 0 (sugared)*/
        "push", "set_sp", "setsp" "jump", "jump_zero", "jump_not_zero",
        "call", "return", "load8", "load4", "load2", "load1", "exit", "store8",
        "store4", "store2", "store1", "sigx8", "sigx4", "sigx2", "sigx1", "add",
        "sub", "mult", "neg", "and", "or", "xor", "not", "pow2", "check_version", "shift_l", "shift_ru",
        "shift_rs", "div_u", "div_s", "rem_u", "rem_s", "lt_u", "lt_s", "lte_u", "lte_s",
        "eq", "gte_u", "gte_s", "gt_u", "gt_s", "read_frame", "read_pixel",
    	"read_char", "put_char", "put_byte", "new_frame", "set_pixel", "add_sample",
	    "data8", "data4", "data2", "data1", "space", "EXPORT", "IMPORT",
        /*but also mnemonics in phase 1 (native)*/
        "exit", "nop", "jump", "jzfwd", "jzback", "setsp", "getpc", "getsp", "push0", "push1",
        "push2", "push4", "push8", "load1", "load2", "load4", "load8", "store1", "store2", "store4",
        "store8", "add", "mult", "div", "rem", "lt", "and", "or", "not", "xor", "pow", "check", "putbyte",
        "readchar", "putchar", "addsample", "setpixel", "newframe", "readpixel", "readframe",
        NULL};
    int is_mnemonic = 0;
    char **ptr = mnemonics;
    while (*ptr != NULL){
        if (!strncmp(*ptr, l, 32)){
            is_mnemonic = 1;
            break;
        }
        ptr++;
    }
    return is_mnemonic;
}

/* The prefix '%' is prepended to a label when it is a mnemonic */
#define MNEMONIC_LABEL_PREFIX(L) (label_is_mnemonic(L)?"%":"")

void print_token_label(char* label){
    int simple_label_in_data = 0;

    if (indata && !gen_init_code) {
       // It's a simple "data8 [label]" statement
       // but label is unknown until run time, so
       // it must be treated as complex data
       start_complex_data();
       simple_label_in_data = 1;
    }

    /* If label is a mnemonic of phase 0, we preappend '%'
       to consider it a label in phase 1 instead of a mnemonic*/
    char *lptr = label;
    if (label[0] == '-' || label[0] == '~') /* Negated or inverted label */
        lptr = &label[1];
    char *ml = MNEMONIC_LABEL_PREFIX(lptr);
    if (!indata || gen_init_code) {
        if (label[0] == '-' ){ /* This label is negated */
            fprintf(out_file, "push8 %s%s\n", ml, &label[1]);
            print_neg();
        } else if (label[0] == '~'){ /* This label is inverted */
            fprintf(out_file, "push8 %s%s\n", ml, &label[1]);
            print_not();
        } else { /* Normal label */
            fprintf(out_file, "push8 %s%s\n", ml, label);
        }
    } else {
    /*** THIS BRANCH IS UNREACHABLE AS LABELS IN DATA ARE NOW COMPLEX DATA ***/
        // Labels are considered always 8-byte sized
        // as represent its value
        printf("data8 [%s]\n", label);
    }

    if (simple_label_in_data) stop_complex_data();
}

/* push! &3 */
void print_token_stack_op_addr(long num){
    fprintf(out_file, "getsp\n");
    long total_depth = op_1st_depth - op_depth +  op_expr_depth + op_list_depth;
    long offset = (num + total_depth) * BYTES_PER_WORD;
    if (offset) { // An offset over SP is required in this case
        // If the negated offset is smaller, put the negated one and negate
        int reqnot = 0;
        if (long_size(~offset) < long_size(offset)) {
            offset = ~offset;
            reqnot = 1; /* Requires negation */
        }
        fprintf(out_file, "push%d %ld #(d=%d, d1=%d, de=%d, dl=%d)\n",
                           long_size(offset), offset,
                           op_depth, op_1st_depth, op_expr_depth, op_list_depth);
        if (reqnot)
            print_not();

        fprintf(out_file, "add\n");
   } else{
        fprintf(out_file, "# push!0 add  #(d=%d, d1=%d, de=%d, dl=%d)\n",
                op_depth, op_1st_depth, op_expr_depth, op_list_depth);
   }
}

/* push! $5 */
void print_token_stack_op_value(long num){
    print_token_stack_op_addr(num);
    fprintf(out_file, "load8\n");
}

/* Add the extra offset accumulated so far */
void print_token_stack_offset(){
    long total_depth = op_1st_depth - op_depth +  op_expr_depth + op_list_depth;
    long word_offset = total_depth;
    if (word_offset){
        fprintf(out_file, "push%d %ld #(d=%d, d1=%d, de=%d, dl=%d)\n",
                           long_size(word_offset), word_offset,
                           op_depth, op_1st_depth, op_expr_depth, op_list_depth);
        fprintf(out_file, "add\n");
    }
    fprintf(out_file, "push1 8\n"); /*words to bytes*/
    fprintf(out_file, "mult\n");
    fprintf(out_file, "add\n");
}


/* Print a not instruction in the current output*/
void print_not(){
    fprintf(out_file, "not\n");
}

/* Print a sign change instruction in the current output*/
void print_neg(){
    //fprintf(out_file, "not     # neg\n"
    //                  "push1 1\n"
    //                  "add\n");
    fprintf(out_file, "push0     # neg\n"
                      "not   \n"
                      "mult  \n");
}

/* Print a sub instruction*/
void print_sub(){
    print_neg();
    fprintf(out_file, "add\n");
}

/* Print an inconditional jump instruction */
void print_jump(){
    fprintf(out_file, "jump\n");
}

/* Print an inconditional jump to label (jump! dst) */
void print_jump_to_label(char *dst){
    fprintf(out_file, "jump %s%s\n", MNEMONIC_LABEL_PREFIX(dst), dst);
}

long call_cnt = 0;

/* Start printing "call! operand" */
void print_start_call() {
    static char label[256];
    sprintf(label, IVM64_BISON_TEMP_LABEL_PREFIX "call_pc_ret_%ld", call_cnt);
    fprintf(out_file, "push8 %s\n", label); /* push return_value (pc_ret) */
}

/* Finish printing "call! operand" */
void print_end_call() {
    static char label[256];
    sprintf(label, IVM64_BISON_TEMP_LABEL_PREFIX "call_pc_ret_%ld", call_cnt++);
    fprintf(out_file, "jump\n"         /*jump*/
                      "%s:\n", label); /*label_pc_ret: */
}

/* Print a call instruction */
void print_call(){
    static char label[256];
    sprintf(label, IVM64_BISON_TEMP_LABEL_PREFIX "call_pc_ret_%ld", call_cnt++);
    fprintf(out_file, "#expand call\n"
                      "getsp\n"     /*push! $0 = call destination*/
                      "load8\n"
                      "push1 <%s\n" /*push! pc_ret */
                      "getsp\n"     /*store8! &2 # = return addr*/
                      "push1 16\n"
                      "add\n"
                      "store8\n"
                      "jump\n"      /* jump*/
                      "%s:\n", label, label);
}

/* Print a call to label instruction (call! dst) */
void print_call_to_label(char *dst){
    static char label[256];
    sprintf(label, IVM64_BISON_TEMP_LABEL_PREFIX "call_pc_ret_%ld", call_cnt++);

    fprintf(out_file, "push1 <%s #near label\n" /*push! pc_ret*/
                      "jump %s%s\n" /*jump! dst*/
                      "%s:\n"
                      ,label, MNEMONIC_LABEL_PREFIX(dst), dst, label);
}

/* Print a return instruction in the current output*/
void print_return(){
    fprintf(out_file, "jump\n");
}

/* Print a jump_zero instruction*/
void print_jump_zero(){
    /*This can be optimized using conditional native instructions
      properly, but it is necessary to know the length and direction
      of jump*/
    fprintf(out_file, "#expand jump_zero\n"
                      "getsp\n"     /* push! &1 */
                      "push1 8\n"
                      "add\n"
                      "load8\n"
                      "jzfwd 3\n"       /* jump_zero! do_jump */
                      "push0\n"         /* jump! do_not_jump */
                      "jzfwd 6\n"
            /* do_jump: */ /*This label is not needed as jz has a fixed offset */
                      "getsp\n"    /* store8! &1 */
                      "push1 8\n"
                      "add \n"
                      "store8\n"
                      "jump\n"     /* jump */
            /* do_not_jump: */ /*This label is not needed as jz has a fixed offset */
                      "getsp\n"    /* store8! &0 = set_sp! &1 := pop */
                      "store8\n"
                      "getsp\n"    /* store8! &0 = set_sp! &1 := pop*/
                      "store8\n");
}

/* Print a jump_zero to label (push! label) instruction*/
void print_jump_zero_to_label(char *dst){
    fprintf(out_file, "jzfwd %s%s\n", MNEMONIC_LABEL_PREFIX(dst), dst);
}

/* Print a jump_not_zero instruction */
void print_jump_not_zero(){
    fprintf(out_file, "#expand jump_not_zero\n"
                      "getsp\n"     /* push! &1 */
                      "push1 8\n"
                      "add\n"
                      "load8\n"
                      "jzfwd 6\n" /* jump_zero! do_not_jump */
            /* do_jump: */ /*This label is not needed as jz has a fixed offset */
                      "getsp\n"    /* store8! &1 */
                      "push1 8\n"
                      "add \n"
                      "store8\n"
                      "jump\n"     /* jump */
            /* do_not_jump: */
                      "getsp\n"    /* store8! &0 = set_sp! &1 */
                      "store8\n"
                      "getsp\n"    /* store8! &0 = set_sp! &1 */
                      "store8\n");
}

/* Print a jump_not_zero to label (push! label) instruction*/
void print_jump_not_zero_to_label(char *dst){
    static long jnz_cnt = 0;
    char label[256];
    sprintf(label, IVM64_BISON_TEMP_LABEL_PREFIX "jnz_%ld", jnz_cnt++);
    fprintf(out_file, "jzfwd <%s #near label\n"
                      "jump %s%s #jz to label\n"
                      "%s:\n"
                      "#jump\n", label, MNEMONIC_LABEL_PREFIX(dst), dst, label);
}

/* Print a sign extension instruction */
void print_sigx(int nbytes){
    if (nbytes < 8){
       int nbits = 8 * nbytes;
       uint64_t x0 = ((1UL << nbits) - 1);
       uint64_t x1 = (1UL << (nbits - 1));
       uint64_t x2 = (-1UL << nbits) + x1;
       fprintf(out_file, "push8 %ld\n" /* and! <x0> */
                         "and\n"
                         "push8 %ld\n" /* xor! <x1> */
                         "xor\n"
                         "push8 %ld\n" /* add! <x1> */
                         "add\n", x0, x1, x2);
    }
}

/* Print a shift left instruction */
void print_shift_l(){
   fprintf(out_file, "pow\n"
                     "mult\n");
}

/* Print a shift right unsigned instruction */
void print_shift_ru(){
   fprintf(out_file, "pow\n"
                     "div\n");
}

/* Print an arithmetic shift right (signed) instruction */
void print_shift_rs(){
    fprintf(out_file,
        /* if n>63, set n=63 */
        "#expand shift_rs\n"
        "push1 63\n" /*lt_u!! 63 $0 */
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "lt\n"
        "jzfwd 7\n"  /*jump_zero! l1 */
        /*l2: */
        "push1 63\n" /*store8!! 63 &0 */
        "getsp\n"
        "push1 8\n"
        "add\n"
        "store8\n"
        /*l1: */
        );
    fprintf(out_file,
        /*-------*/
        "getsp\n"   /*shift_ru!! $1 $0*/
        "push1 8\n"
        "add\n"
        "load8\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "pow\n"
        "div\n"
        "push1 63 pow\n" /*push! 0x800...0*/
        "getsp\n"    /*and! $3 */
        "push1 24\n"
        "add\n"
        "load8\n"
        "and\n"
        "getsp\n"    /*shift_ru!! $0 $2 */
        "load8\n"
        "getsp\n"
        "push1 24\n"
        "add\n"
        "load8\n"
        "pow\n"
        "div\n"
        "not\n"     /*sub*/
        "push1 1\n"
        "add\n"
        "add\n"
        "push1 2\n" /* mult! 2*/
        "mult\n"
        "add\n"     /*add*/
        "getsp\n"   /*store8! &1*/
        "push1 8\n"
        "add\n"
        "store8\n"  /*store8! &1*/
        "getsp\n"
        "push1 8\n"
        "add\n"
        "store8\n");
}

/* Print an arithmetic shift right (signed) in a constant amount*/
void print_shift_rs_const(uint64_t a){
    if (a>=64) {
        a = 63;
    }

    fprintf(out_file,
        /*-------*/
        "#expand shift_rs const\n"
        "push1 %ld\n" /* shift_ru! a */
        "pow\n"
        "div\n"
        /*-------*/
        "getsp\n"    /* shift_ru!! $0 63-a */
        "load8\n"
        "push1 %ld\n"
        "pow\n"
        "and\n"
        "push0\n"
        "not\n"
        "mult\n"
        "or\n"       ,a, 63-a);
}

void print_div_s(){
    /* This operation requires further optimization */
    /* signed a/b = abs(a)/abs(b) * sign(a xor b)
      (do not use sign(a*b) instead of xor, as overflow is possible)*/
    fprintf(out_file,
        /* abs(a) */
        "#expand div_s\n"
        "getsp\n"    /*div_u!! $1 0x80...0 */
        "push1 8\n"
        "add\n"
        "load8\n"
        "push8 0x8000000000000000\n"
        "div\n"
        "getsp\n"   /*push! $0*/
        "load8\n"
        "not\n"     /*neg*/
        "push1 1\n"
        "add\n"
        "pow\n"     /*pow*/
        "not\n"     /*sub*/
        "push1 1\n"
        "add\n"
        "add\n"
        "not\n"     /*neg*/
        "push1 1\n"
        "add\n"
        "getsp\n"   /*mult! $2 */
        "push1 16 \n"
        "add\n"
        "load8\n"
        "mult\n"
        /* abs(b) */
        "getsp\n"   /* Repeat again */
        "push1 8 \n"
        "add\n"
        "load8\n"
        "push8 0x8000000000000000\n"
        "div\n"
        "getsp\n"
        "load8\n"
        "not\n"
        "push1 1\n"
        "add\n"
        "pow\n"
        "not\n"
        "push1 1\n"
        "add\n"
        "add\n"
        "not\n"
        "push1 1\n"
        "add\n"
        "getsp\n"
        "push1 16 \n"
        "add\n"
        "load8\n"
        "mult\n"
        /* abs(a)/abs(b) */
        "div\n"     /*div_u*/
        /* sign(a xor b) */
        "getsp\n"   /*xor!! $1 $2 */
        "push1 8 \n"
        "add\n"
        "load8\n"
        "getsp\n"
        "push1 24 \n"
        "add\n"
        "load8\n"
        "xor\n"    /*xor*/
        "push8 0x8000000000000000\n" /*div_u! 0x80...0 */
        "div\n"
        "getsp\n"  /*push! $0*/
        "load8\n"
        "not\n"    /*neg*/
        "push1 1\n"
        "add\n"
        "pow\n"    /*pow2*/
        "not\n"    /*sub*/
        "push1 1\n"
        "add\n"
        "add\n"
        "not\n"    /*neg*/
        "push1 1\n"
        "add\n"
        "mult\n"   /*mult*/
        /* store result, clean stack */
        "getsp\n"  /*store8! &2*/
        "push1 16 \n"
        "add\n"
        "store8\n"
        "getsp\n"  /*store8! &0*/
        "store8\n");
}

void print_rem_s(){
    /* This operation requires further optimization */
    /* signed a%b = abs(a)%abs(b) * sign(a) */
    fprintf(out_file,
        /* abs(a) */
        "#expand rem_s\n"
        "getsp\n"    /*div_u!! $1 0x80..0 */
        "push1 8\n"
        "add\n"
        "load8\n"
        "push8 0x8000000000000000\n"
        "div\n"
        "getsp\n"   /*push! $0*/
        "load8\n"
        "not\n"     /*neg*/
        "push1 1\n"
        "add\n"
        "pow\n"     /*pow*/
        "not\n"     /*sub*/
        "push1 1\n"
        "add\n"
        "add\n"
        "not\n"     /*neg*/
        "push1 1\n"
        "add\n"
        "getsp\n"   /*mult! $2 */
        "push1 16 \n"
        "add\n"
        "load8\n"
        "mult\n"
        /* abs(b) */
        "getsp\n"   /* Repeat again */
        "push1 8 \n"
        "add\n"
        "load8\n"
        "push8 0x8000000000000000\n"
        "div\n"
        "getsp\n"
        "load8\n"
        "not\n"
        "push1 1\n"
        "add\n"
        "pow\n"
        "not\n"
        "push1 1\n"
        "add\n"
        "add\n"
        "not\n"
        "push1 1\n"
        "add\n"
        "getsp\n"
        "push1 16 \n"
        "add\n"
        "load8\n"
        "mult\n"
        /* abs(a) % abs(b) */
        "rem\n"     /*rem_u*/
        /* sign(a) */
        "getsp\n"   /*push! $2 */
        "push1 16\n"
        "add\n"
        "load8\n"
        "push8 0x8000000000000000\n" /*div_u! 0x80...0 */
        "div\n"
        "getsp\n"  /*push! $0*/
        "load8\n"
        "not\n"    /*neg*/
        "push1 1\n"
        "add\n"
        "pow\n"    /*pow2*/
        "not\n"    /*sub*/
        "push1 1\n"
        "add\n"
        "add\n"
        "not\n"    /*neg*/
        "push1 1\n"
        "add\n"
        "mult\n"   /*mult*/
        /* store result, clean stack */
        "getsp\n"  /*store8! &2*/
        "push1 16 \n"
        "add\n"
        "store8\n"
        "getsp\n"  /*store8! &0*/
        "store8\n");
}

void print_lt_s(){
    fprintf(out_file,
        "#expand lt_s\n"
        "push1 63\n"
        "pow\n"
        "add\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "push1 63\n"
        "pow\n"
        "add\n"
        "getsp\n"
        "push1 16\n"
        "add\n"
        "store8\n"
        "lt\n");
}

void print_lte_u(){
    fprintf(out_file,
        "#expand lte_u\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "lt\n"
        "getsp\n"
        "push1 8\n"
        "add  \n"
        "store8\n"
        "push1 1\n"
        "lt\n");
}

void print_lte_s(){
    fprintf(out_file,
        "#expand lte_s\n"
        "push1 63\n"
        "pow\n"
        "add\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "push1 63\n"
        "pow\n"
        "add\n"
        "getsp\n"
        "push1 16\n"
        "add\n"
        "store8\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "lt\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "store8\n"
        "push1 1\n"
        "lt\n");
}

void print_eq(){
   fprintf(out_file, "#expand eq\n"
                     "xor\n"
                     "push1 1\n"
                     "lt\n");
}

void print_gt_u(){
    fprintf(out_file,
        "#expand gt_u\n"
        "getsp\n"    /*push! $1 */
        "push1 8\n"
        "add\n"
        "load8\n"
        "lt\n"      /*lt_u*/
        "getsp\n"   /*store8! &1 */
        "push1 8\n"
        "add\n"
        "store8\n"
    );
}

void print_gt_s(){
    fprintf(out_file,
        "#expand gt_s\n"
        "push1 63\n"
        "pow\n"
        "add\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "push1 63\n"
        "pow\n"
        "add\n"
        "getsp\n"
        "push1 16\n"
        "add\n"
        "store8\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "lt\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "store8\n"
    );
}

void print_gte_u(){
    fprintf(out_file,
        "#expand gte_u\n"
        "lt\n"
        "push1 1\n"
        "lt\n");
}

void print_gte_s(){
    fprintf(out_file,
        "#expand gte_s\n"
        "push1 63\n"
        "pow\n"
        "add\n"
        "getsp\n"
        "push1 8\n"
        "add\n"
        "load8\n"
        "push1 63\n"
        "pow\n"
        "add\n"
        "getsp\n"
        "push1 16\n"
        "add\n"
        "store8\n"
        "lt\n"
        "push1 1\n"
        "lt\n");
}

void print_check_version(){
    fprintf(out_file,
        "#expand check_version\n"
        "push1 %d\n"
        "check\n"
        , IVM_BINARY_VERSION);
}

void print_num_data_repetition(long num, long N){
    for (long k=0; k<N; k++){
        print_token_num(num);
    }
}

void start_complex_data(){
   if (indata && !gen_init_code){
       cdata++;
       fprintf(out_file, ".LIVM64_bison_complex_data%ld:\n", cdata);
       fprintf(out_file, "\tdata%d [0] # complex data\n", indata);
       gen_init_code = 1;
       out_file = init_file;
   }
}

void stop_complex_data(){
   /* When going out of the expresion in data mode, stop generating code*/
   if (0 == paren_expr_depth){
       fprintf(out_file, "#store8! .LIVM64_bison_complex_data%ld # complex data \n", cdata);
       fprintf(out_file, "  push8 .LIVM64_bison_complex_data%ld\n", cdata);
       if (indata)
           fprintf(out_file, "  store%d\n", indata);
       else
           fprintf(out_file, "  store8\n");
       gen_init_code = 0;
       out_file = stdout;
   }
}

/* To be called after the ')' closing one binary expression */
void close_bin_expr(const char *mnemonic){
    fprintf(out_file, "%s # bin. expr.\n", mnemonic);
    op_expr_depth--;
    paren_expr_depth--;
}

/* To be called after the ')' closing one unary expression */
void close_unary_expr(const char *mnemonic){
    fprintf(out_file, "%s # unry. expr.\n", mnemonic);
    paren_expr_depth--;
}

/* Print a space directive generating the code to initialize
   its associated pointer, when it is a simple numeric operand, like "space 100" */
void print_space_num(long nbytes){
    /* An internal label for this space directive */
    static char space_ptr[256];
    static long csym = 0;
    sprintf(space_ptr, ".LIVM64_bison_space%ld", csym);
    /* Output file*/
    fprintf(out_file, "%s:\n"
                      "\tdata8 [0]\n", space_ptr);

    /* Initialization file*/
    fprintf(init_file, "# Init code for space %ld\n"
                       "push8 .LIVM64_bison_last_position\n" /* Here the program ends */
                       /*--*/
                       "push8 .LIVM64_bison_last_position\n" /* Length of the arg. file */
                       "load8\n"
                       "add\n"
                       /*--*/
                       "push8 .LIVM64_bison_space_bytes\n" /* bytes of the space area occupied so far */
                       "load8\n"
                       "add\n"
                       "push8 %s\n"
                       "store8\n\n"    /* store the pointer of this space in the area */
                       ,csym, space_ptr);

    /* Increase space byte counter */
    fprintf(init_file,
                      "push8 .LIVM64_bison_space_bytes\n"
                      "load8\n"
                      "push8 %ld\n"
                      "add\n"
                      "push8 .LIVM64_bison_space_bytes\n"
                      "store8\n\n"
                      ,nbytes);
    csym++;
}

/* Print a space directive generating the code to initialize its associated pointer, when it
   is a complex expression, like "space (+ label1 -label2)".
   Note that a complex expression has been allocated previously to store the pointer (like data8),
   and that initially this allocated position contains the value of the number of bytes to allocate
   by this space directive */
void print_space_expr(long cdata){
    /* The internal label for this space directive is the one of its associated complex data */
    static char space_ptr[256];
    sprintf(space_ptr, ".LIVM64_bison_complex_data%ld\n", cdata);

    /* Initialization file*/
    fprintf(init_file, "\n"
                       "# Init code for space complex expression %ld\n"
                       /* At this point, the complex data contains the number of bytes, which was written
                          during its initilizialization (nbytes)*/
                       "push8 %s\n" /* preserve the number of bytes to allocate*/
                       "load8\n"
                       /*--*/
                       "push8 .LIVM64_bison_last_position\n" /* here the program ends */
                       /*--*/
                       "push8 .LIVM64_bison_last_position\n" /* length of the arg. file */
                       "load8\n"
                       "add\n"
                       /*--*/
                       "push8 .LIVM64_bison_space_bytes\n" /* bytes of the space area occupied so far */
                       "load8\n"
                       "add\n"
                       "push8 %s\n"
                       "store8\n\n"    /* store the pointer of this space in the area */
                       ,cdata, space_ptr, space_ptr);

    /* Increase space byte counter */
    fprintf(init_file,
                      "push8 .LIVM64_bison_space_bytes\n"
                      "load8\n"
                      "add\n" /* Add nbytes, which was preserved previously in the stack */
                      "push8 .LIVM64_bison_space_bytes\n"
                      "store8\n\n"
                      );

}
#endif /*AS0*/


// CODE FOR PHASE 1 ASSEMBLER ========================================================
#ifdef IVM64_BISON_AS_1

/* This struct represents the binary program.
   The pos variable is the size of the current
   program written so far, i.e. the current PC */
typedef struct binary{
    uint8_t *content;       /* The binary, as an array of bytes */
    unsigned long pos;      /* Current PC */
    unsigned long cur_size; /* Current size in bytes */
} binary_t;

binary_t Binprog;

void init_bin(unsigned long size){
   Binprog.pos = 0;
   Binprog.content = (uint8_t *)malloc(size*sizeof(uint8_t)+1);
   Binprog.cur_size = size;
}

void write_bytes(int nbytes, uint64_t value){
    if (nbytes > sizeof(uint64_t)){
        fprintf(stderr, "cannnot write more than 8 bytes\n");
        exit(EXIT_FAILURE);
    }
    if (Binprog.pos + nbytes + 8 > Binprog.cur_size) {
        /* Need to realloc */
        Binprog.cur_size = Binprog.cur_size * 2;
        Binprog.content = (uint8_t *)realloc(Binprog.content,
                                             Binprog.cur_size*sizeof(uint8_t));
    }
    for (int i=0; i<nbytes; i++){
        Binprog.content[Binprog.pos++] = value & 0xff; /* little endian ? */
        value >>= 8;
    }
}

// -------------------------------------------------------
// Hashed symbol table based on the simple list symbol table
// from: Compiler Construction using Flex and Bison, Anthony A. Aaby, 2003
// https://dlsiis.fi.upm.es/traductores/Software/Flex-Bison.pdf
struct symrec_s
{
    char *name;  /* name of label (symbol)*/
    long pc;     /* position associated to the label */
    char *alias; /* if not null, this symbol is an alias of another label;
                    this is the other label; in this case pc has no meaning */
    int absolute;/* If true and the pc value must be treated as an absolute value
                    not relative to the start of the program. Used for numeric
                    alias; the field alias must be null in this case. */
    int pass;    /* Pass number where this symbol is added or updated */
    int inner;   /* If true, this is an inner instrumental label, and
                    it is not dumped to the .sym file */
    struct symrec_s *next; /* link field */
};
typedef struct symrec_s symrec;

typedef struct sym_table_s {
    /* Symbol hash table, each entry is a pointer to
       a symbol linked list of symrec */
    /* sym_table -> sym_table[0] -> sym_rec -> sym_rec ....
                    sym_table[1] -> sym_rec -> sym_rec ....
                    ... */
    symrec** sym_table;
    unsigned long int sym_table_size;
} sym_table_t;

static sym_table_t* init_symtable(unsigned long size){
   sym_table_t *T = (sym_table_t*)malloc(sizeof(sym_table_t));
   T->sym_table = (symrec**)malloc(size*sizeof(symrec*));
   for (long i=0; i<size; i++){
        T->sym_table[i] = (symrec*)0;
   }
   T->sym_table_size = size;
   return T;
   // fprintf(stderr, "Symbol table initialized with %ld entries\n", sym_table_size); //debug
}

/* Deallocate all records in the list for each
   not empty table entry */
void destroy_symtable(sym_table_t **TT){
    /*TODO: finish implementing this*/
    if (!TT) return;
    sym_table_t *T = *TT;
    if (!T) return;
    for (long i=0; i < T->sym_table_size; i++){
        symrec *p, *q;
        p = T->sym_table[i];
        if (p) {
            for (q = p; p != NULL; p = q){
                q = p->next;
                free(p);
            }
        }
    }
    free(T->sym_table);
    free(T);
    *TT = NULL;
    return;
}

/*getsym which returns a pointer to the symbol table entry corresponding
  to an identifier*/
symrec* getsym(char *sym_name, symrec* sym_table)
{
    symrec *ptr;
    for (ptr = sym_table; ptr != (symrec *)0; ptr = (symrec *)ptr->next)
        if (strcmp (ptr->name, sym_name) == 0)
            return ptr;
    return 0;
}

/*Two operations: putsym to put an identifier into the table*/
/* redefined can be 0 = new symbol
                    1 = symbol is updated in a different pass
                    2 = symbol is redefined (updated in the same pass)
                    3 = erroneous update (using a past pass)
*/
symrec* putsym(char *sym_name, long pc, char *alias, int absolute, int pass, symrec** sym_table, int *redefined)
{
    symrec *ptr;
    if (ptr = getsym(sym_name, *sym_table)) {
        /* Node exists */
        if (ptr->alias) free(ptr->alias);
        if (pass > ptr->pass)       *redefined = 1;
        else if (pass == ptr->pass) *redefined = 2;
        else                        *redefined = 3;
    } else {
        /* New node */
        ptr = (symrec *) malloc (sizeof(symrec));
        ptr->next = (struct symrec_s *)(*sym_table);
        ptr->name = strdup(sym_name);
        *sym_table = ptr;
        *redefined = 0;
    }
    ptr->pc = pc;
    ptr->absolute = absolute;
    ptr->pass = pass;
    if (alias){
        ptr->alias = strdup(alias);
    } else {
        ptr->alias = NULL;
    }
    ptr->inner = 0; /* Not inner by default*/
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
symrec* putsym_hash(sym_table_t *T, char *sym_name, long pc, char* alias, int absolute, int pass, int *redefined){
    if (!T) return NULL;
    long hash = hashfun(sym_name, T->sym_table_size);
    return putsym(sym_name, pc, alias, absolute, pass, &T->sym_table[hash], redefined);
}

symrec* getsym_hash(sym_table_t *T, char *sym_name){
    if (!T) return NULL;
    static int nrec = 0;
    long hash = hashfun(sym_name, T->sym_table_size);
    symrec* r = getsym(sym_name, T->sym_table[hash]);
    //if (r) fprintf(stderr, "r->name=%s, value=%ld, alias=%s, absolute=%d\n", r->name, r->pc, r->alias, r->absolute);
    if (r && r->alias){
        if (nrec++ > 50) {
            fprintf(stderr, "Error: alias recursion reached for symbol '%s'\n", sym_name);
            exit(EXIT_FAILURE);
        }
        r = getsym_hash(T, r->alias);
    }
    nrec = 0;
    return r;
}

long print_symtable(sym_table_t *T, int verbose)
{
    symrec *p;
    long nsym = 0;
    for (long i=0; i < T->sym_table_size; i++){
        for (p = T->sym_table[i]; p != (symrec *)0; p = (symrec *)p->next){
            if (verbose){
                if (!p->alias && !p->absolute) {
                    // Normal label
                    fprintf(stderr, "PC=%ld symbol=%s\n", p->pc, p->name);
                } else if (p->absolute) {
                    // This is a numeric alias
                    fprintf(stderr, "NUMERIC ALIAS '%s' ---> %ld\n", p->name, p->pc);
                } else {
                    // This is an alias
                    fprintf(stderr, "ALIAS '%s' ---> '%s'\n", p->name, p->alias);
                }
            }
            nsym++;
        }
    }
    return nsym;
}

/* Dump the binary */
long dump_bin(char *filename){
    long n;
    FILE* fd = fopen(filename, "w"); /* This is our output */
    if (!fd) {
        fprintf(stderr, "Error opening binary output file'%s'\n", filename);
        exit(EXIT_FAILURE);
    }
    n = fwrite(Binprog.content, 1, Binprog.pos, fd); // Note that 'Binprog.pos' points to the next byte to be written
    fclose(fd);
    return n;
}

/* Dump the symbol file in the format of ivm implementation*/
void dump_sym_file(sym_table_t *T, char *filename)
{
    FILE* fd = fopen(filename, "w"); /* This is our output */
    if (!fd) {
        fprintf(stderr, "Error opening symbol output file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    fprintf(fd, "--Previous--\n\n");
    fprintf(fd, "--Size--\n%ld\n", Binprog.pos);
    fprintf(fd, "--Relative--\n");
    fprintf(fd, "--Constant--\n");
    fprintf(fd, "--Labels--\n");
    symrec *p;
    long nsym = 0;
    for (long i=0; i < T->sym_table_size; i++){
        for (p = T->sym_table[i]; p != (symrec *)0; p = (symrec *)p->next){
            if (p->inner) continue; /* Inner labels are not dumped */
            if (!p->alias && !p->absolute) {
                // Normal label
                if (!strncmp(IVM64_BISON_TEMP_LABEL_PREFIX, p->name, strlen(IVM64_BISON_TEMP_LABEL_PREFIX))) continue; /* Auxiliary labels are not dumped either */
                fprintf(fd, "z/%s\t%ld\n", p->name, p->pc);
            } else {
                // This is a label or numeric alias: do nothing
            }
            nsym++;
        }
    }
    fprintf(fd, "--Spacers--\n");
    if (fd) fclose(fd);
}

// -------------------------------------------------------
// Simple list for solving pending references (labels used
// in the progam before its declaration)
struct labelrec
{
    char *name;        /* name of the pending label*/
    int64_t pos;       /* where to write the value */
    int64_t pc_offset; /* offset to add to get the value */
    int size;          /* size of the label: 8, 4, 2, 1 (8 by default) */
    struct labelrec *next; /* link field */
};

typedef struct labelrec labelrec;
labelrec* pending_refs = (labelrec*)0;

/* Add one element to list */
labelrec* add_pending_ref(char *name, long pos, long pc_offset, int size)
{
    labelrec *p;
    p = (labelrec *) malloc(sizeof(labelrec));
    p->name = (char *) malloc(strlen(name)+1);
    strcpy(p->name, name);
    p->pos = pos;
    p->pc_offset = pc_offset;
    p->size = size;
    p->next = (struct labelrec *)(pending_refs);
    pending_refs = p;
    return p;
}

/* Print error message and exit if symbol p with name 'name' is not defined */
void print_error_symbol_not_declared(symrec* p, char* name){
     if (!p) {
         fprintf(stderr, "Error: symbol '%s' not declared\n", name);
         exit(EXIT_FAILURE);
     }
}

/* Solve all pending reference (used when parsing with no optimization) */
void solve_pending_references(sym_table_t *T){
    long nref=0;
    /* Traverse the list of pending references, and update them
       with the known value of its associated label */
    for (labelrec* p = pending_refs; p != (labelrec *)0; p = (labelrec *)p->next){
         symrec* q = getsym_hash(T, p->name);
         if (!q) {
             fprintf(stderr, "Error: symbol '%s' not declared\n", p->name);
             exit(EXIT_FAILURE);
         } else {
             void *offset = (void*)&Binprog.content[p->pos]; /*Valid only for little endian?*/
             #if IVM64_BISON_VERBOSE == 1
             fprintf(stderr, "pending @ pos=%ld symbol=%s (=%ld)  offset=%ld\n", p->pos, p->name, q->pc, p->pc_offset);
             #endif
             if (!q->absolute){
                 // This is a standard offset (pc associated to label + the saved offset over pc)
                 int sz = p->size;
                 if (8 == sz) *(uint64_t*)offset = (uint64_t)(q->pc + p->pc_offset);
                 else if (4 == sz) *(uint32_t*)offset = (uint32_t)(q->pc + p->pc_offset);
                 else if (2 == sz) *(uint16_t*)offset = (uint16_t)(q->pc + p->pc_offset);
                 else if (1 == sz) *(uint8_t*)offset = (uint8_t)(q->pc + p->pc_offset);
             } else {
                 // This is a pending numeric alias:
                 // suppress PC-relative offset computation using NOPs
                 // (this must be used only when optimization is disabled)
                 *((uint8_t*)offset-2) = 0x01;
                 *(uint64_t*)offset = q->pc;
                 *((uint8_t*)offset+8) = 0x01;
             }
         }
         nref++;
    }
    #if IVM64_BISON_VERBOSE == 1
    fprintf(stderr, "Solved %ld pending references\n", nref);
    #endif
}


#endif /*AS1*/


