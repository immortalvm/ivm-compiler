%{
    /* IVM64 assembler
       Parser phase 0
       Author: Eladio Gutierrez
               University of Malaga, Spain
       Jan 2022
    */

    #define IVM64_BISON_AS_VERSION "v2.1"
    #define IVM64_ARCHITECTURE "ivm-v2.1"

    #include <stdio.h>
    int yylex(void);
    void yyerror(const char*);
    extern int noerror; /*Defined in .l*/

    #define YYERROR_VERBOSE 1
    extern int yylineno;

    #define YYMAXDEPTH 1000000

    #define INITIALIZATION_FILE "_ivm64_bison_init_code" /* Prefix to init file name */
    char *init_filename = NULL; /* Temporary initialization file to be generated */

    /* Depths of stack operands in a multioperand instruction
            op_depth -> depth of the operand in the analysis
            op_first_depth -> depth of the first operand (leftmost)
       The effective offset of each operand is
       (op_first_depth - op_depth)*BYTES_PER_WORD

       E.g., push!!! &1 &2 &3
             &1 -> op_depth=3, op_1st_depth=3
             &2 -> op_depth=2, op_1st_depth=3
             &3 -> op_depth=1, op_1st_depth=3
    */
    int op_depth = 0;
    int op_1st_depth = 0;

    /* Operand depth for expressions, which actually use
       prefix notation, e.g. "(+ 3 &2)" */
    int op_expr_depth = 0;

    /* Parenthesis depth in expressions */
    int paren_expr_depth = 0;

    /* Operand stack depth for *[op op op] list form */
    int op_list_depth = 0;

    /* If 0 a data directive is NOT being parsed;
       If not 0, a data directive of size indata is being parsed
       For example, if parsing "data4 [-1 20]", indata=4
    */
    int indata = 0;

    /* If true the code associated to an expression operand
       (like "(+ &1 label)) is generated and saved as an
       initialization code, but not printed inmmediately
       It's used when processing data directives that can
       contain expressions
    */
    /* True when an prefix expression operand starts and we
       are in data mode*/
    int gen_init_code = 0;

    /* File to store the initialization code fragments */
    FILE* init_file;

    /* Where to write, in stdout or initialization file*/
    FILE* out_file;

    /* Count complex data processed */
    long cdata = 0;

    //~ /* Bytes to allocate in the space area */
    //~ /* At least 8 for store the length of this area
    //~    at the beginning of the area */
    //~ long space = 8;

    #define IVM64_BISON_AS_0
    #include "ivm64_helper_bison.c"

%}

/* These declare our output file names. */
/* This is not POSIX compliant */
//%output "y.tab.0.c"
//%defines "y.tab.0.h"

/* The field names of this union are used as return types for
   the tokens both in the lexer and the parser*/
%union {
    long int    num;
    int         opcode;
    char       *label;
}

/* White space */
%token TOKEN_WS

/* Operands */
%token <num>TOKEN_NUM
%token <label>TOKEN_LABEL
%token <label>TOKEN_LABEL_DECL
%token <num>TOKEN_STACK_OP_ADDR
%token <num>TOKEN_STACK_OP_VALUE

/* Instructions */
%token TOKEN_PUSH
%token TOKEN_SET_SP
%token TOKEN_JUMP
%token TOKEN_JUMP_ZERO
%token TOKEN_JUMP_NOT_ZERO
%token TOKEN_CALL
%token TOKEN_RETURN
%token TOKEN_LOAD8
%token TOKEN_LOAD4
%token TOKEN_LOAD2
%token TOKEN_LOAD1
%token TOKEN_SIGX8
%token TOKEN_SIGX4
%token TOKEN_SIGX2
%token TOKEN_SIGX1
%token TOKEN_STORE8
%token TOKEN_STORE4
%token TOKEN_STORE2
%token TOKEN_STORE1
%token TOKEN_ADD
%token TOKEN_SUB
%token TOKEN_MULT
%token TOKEN_NEG
%token TOKEN_AND
%token TOKEN_OR
%token TOKEN_XOR
%token TOKEN_NOT
%token TOKEN_POW2
%token TOKEN_CHECK_VERSION
%token TOKEN_SHIFT_L
%token TOKEN_SHIFT_RU
%token TOKEN_SHIFT_RS
%token TOKEN_DIV_U
%token TOKEN_DIV_S
%token TOKEN_REM_U
%token TOKEN_REM_S
%token TOKEN_LT_U
%token TOKEN_LT_S
%token TOKEN_LTE_U
%token TOKEN_LTE_S
%token TOKEN_EQ
%token TOKEN_GTE_U
%token TOKEN_GTE_S
%token TOKEN_GT_U
%token TOKEN_GT_S
%token TOKEN_READ_FRAME
%token TOKEN_READ_PIXEL
%token TOKEN_READ_CHAR
%token TOKEN_PUT_CHAR
%token TOKEN_PUT_BYTE
%token TOKEN_NEW_FRAME
%token TOKEN_SET_PIXEL
%token TOKEN_ADD_SAMPLE
%token TOKEN_EXIT

/* Specific operators tor for expressions */
/* Binary eq is '=' */
%token TOKEN_EXPR_LT_U
%token TOKEN_EXPR_LT_S
%token TOKEN_EXPR_LTE_U
%token TOKEN_EXPR_LTE_S
%token TOKEN_EXPR_GT_U
%token TOKEN_EXPR_GT_S
%token TOKEN_EXPR_GTE_U
%token TOKEN_EXPR_GTE_S
%token TOKEN_EXPR_SHIFT_L
%token TOKEN_EXPR_SHIFT_RS
%token TOKEN_EXPR_SHIFT_RU
%token TOKEN_EXPR_DIV_U
%token TOKEN_EXPR_DIV_S
%token TOKEN_EXPR_REM_U
%token TOKEN_EXPR_REM_S


/* Directives */
%token TOKEN_DATA8
%token TOKEN_DATA4
%token TOKEN_DATA2
%token TOKEN_DATA1
%token TOKEN_SPACE
%token TOKEN_EXPORT
%token TOKEN_IMPORT

%token <label>TOKEN_ALIAS

%token TOKEN_UNKNOWN

/* Type of some non-terminal symbols */
%type <num>num_operand
%type <label>label_operand
%type <label>mnemonic_as_label
%type <label>alias_decl_not_supported

%%

/* A valid assembly program is made of a
   sequence of valid statements */
assembler: prog  {/* Valid program */}

prog:
    /* empty */
  | prog statement
  ;

statement:
    insn
  | directive
  | alias
  ;

insn:
    TOKEN_LABEL_DECL                    {fprintf(out_file, "%s: #------\n", $1); }
  /* Phase 0 instructions with direct correspondences to ivm native ISA */
  | TOKEN_EXIT                          {fprintf(out_file, "exit\n")  ;}
  | TOKEN_PUSH   operand_sequence       {fprintf(out_file, "#push #nop\n") ;}
  | TOKEN_SET_SP     operand_sequence   {fprintf(out_file, "setsp\n") ; /* Note that the native instruction is "setsp"
                                                                  but sugared assembly uses "set_sp"*/}
  | TOKEN_LOAD8  operand_sequence       {fprintf(out_file, "load8\n") ;}
  | TOKEN_LOAD4  operand_sequence       {fprintf(out_file, "load4\n") ;}
  | TOKEN_LOAD2  operand_sequence       {fprintf(out_file, "load2\n") ;}
  | TOKEN_LOAD1  operand_sequence       {fprintf(out_file, "load1\n") ;}
  | TOKEN_STORE8  operand_sequence      {fprintf(out_file, "store8\n") ;}
  | TOKEN_STORE4  operand_sequence      {fprintf(out_file, "store4\n") ;}
  | TOKEN_STORE2  operand_sequence      {fprintf(out_file, "store2\n") ;}
  | TOKEN_STORE1  operand_sequence      {fprintf(out_file, "store1\n") ;}
  | TOKEN_ADD  operand_sequence         {fprintf(out_file, "add\n") ;}
  | TOKEN_MULT  operand_sequence        {fprintf(out_file, "mult\n") ;}
  | TOKEN_DIV_U  operand_sequence       {fprintf(out_file, "div\n") ; /*difference in name*/}
  | TOKEN_REM_U  operand_sequence       {fprintf(out_file, "rem\n") ; /*difference in name*/}
  | TOKEN_AND  operand_sequence         {fprintf(out_file, "and\n") ;}
  | TOKEN_OR  operand_sequence          {fprintf(out_file, "or\n") ;}
  | TOKEN_XOR  operand_sequence         {fprintf(out_file, "xor\n") ;}
  | TOKEN_NOT  operand_sequence         {fprintf(out_file, "not\n") ;}
  | TOKEN_POW2  operand_sequence        {fprintf(out_file, "pow\n") ;     /*difference in name*/}
  | TOKEN_LT_U  operand_sequence        {fprintf(out_file, "lt\n") ;}
  | TOKEN_READ_FRAME  operand_sequence  {fprintf(out_file, "readframe\n") /*difference in name*/ ;}
  | TOKEN_READ_PIXEL  operand_sequence  {fprintf(out_file, "readpixel\n") /*difference in name*/ ;}
  | TOKEN_READ_CHAR                     {fprintf(out_file, "readchar\n")  /*difference in name*/ ;}
  | TOKEN_PUT_CHAR  operand_sequence    {fprintf(out_file, "putchar\n")   /*difference in name*/ ;}
  | TOKEN_PUT_BYTE  operand_sequence    {fprintf(out_file, "putbyte\n")   /*difference in name*/ ;}
  | TOKEN_NEW_FRAME  operand_sequence   {fprintf(out_file, "newframe\n")  /*difference in name*/ ;}
  | TOKEN_SET_PIXEL  operand_sequence   {fprintf(out_file, "setpixel\n")  /*difference in name*/ ;}
  | TOKEN_ADD_SAMPLE  operand_sequence  {fprintf(out_file, "addsample\n") /*difference in name*/ ;}
  /* Phase 0 instructions that are actually pseudoinstructions*/
  | TOKEN_RETURN  operand_sequence      {print_return();}
  | TOKEN_SIGX8  operand_sequence       {}
  | TOKEN_SIGX4  operand_sequence       {print_sigx(4);}
  | TOKEN_SIGX2  operand_sequence       {print_sigx(2);}
  | TOKEN_SIGX1  operand_sequence       {print_sigx(1);}
  | TOKEN_SUB  operand_sequence         {print_sub();}
  | TOKEN_NEG  operand_sequence         {print_neg();}
  | TOKEN_SHIFT_L  operand_sequence     {print_shift_l();}
  | TOKEN_SHIFT_RU  operand_sequence    {print_shift_ru();}
  | TOKEN_DIV_S  operand_sequence       {print_div_s();}
  | TOKEN_REM_S  operand_sequence       {print_rem_s();}
  | TOKEN_LT_S  operand_sequence        {print_lt_s();}
  | TOKEN_LTE_U  operand_sequence       {print_lte_u();}
  | TOKEN_LTE_S  operand_sequence       {print_lte_s();}
  | TOKEN_EQ  operand_sequence          {print_eq();}
  | TOKEN_GTE_U  operand_sequence       {print_gte_u();}
  | TOKEN_GTE_S  operand_sequence       {print_gte_s();}
  | TOKEN_GT_U  operand_sequence        {print_gt_u();}
  | TOKEN_GT_S  operand_sequence        {print_gt_s();}
  | TOKEN_CHECK_VERSION                 {print_check_version();}
  /* Special cases that are optimized*/
  | shift_rs_instruction
  | jump_instruction
  | call_instruction
  | jump_zero_instruction
  | jump_not_zero_instruction
  ;

/* Note that shift_rs, jumps and calls are optimized only
   for operand sequences with '!', not for '*[]' (form2) */

/* "shift_rs! constant" is handled specially for efficiency */
shift_rs_instruction:
    TOKEN_SHIFT_RS                                    {print_shift_rs();}
  | TOKEN_SHIFT_RS  '!'  TOKEN_NUM                    {print_shift_rs_const($3);}
  | TOKEN_SHIFT_RS  '!'  one_non_numeric_operand      {print_shift_rs();}
  | TOKEN_SHIFT_RS  '!' '!' two_operands              {print_shift_rs();}
  | TOKEN_SHIFT_RS  not_empty_operand_sequence_form2  {print_shift_rs();}

/* Although native jump has no operands in order to optimize
   it, we consider zero or one operand */
jump_instruction:
    TOKEN_JUMP                                   {print_jump();}
  | TOKEN_JUMP  '!' TOKEN_LABEL                  {print_jump_to_label($3);}
  | TOKEN_JUMP  '!' one_jump_operand             {print_jump();}
  | TOKEN_JUMP  not_empty_operand_sequence_form2 {print_jump();}
  ;

/* Instructions jump_zero/jump_not_zero/call:
       can have none, one or two operands
   Do not forget to update the extra offsets.
   When the operand is a label it is handled as a special case.
   It is still inefficient for labels that are mnemonics, as they are pushed
   with the '%' prefix. */
jump_zero_instruction:
    TOKEN_JUMP_ZERO                            {print_jump_zero();}
  | TOKEN_JUMP_ZERO  '!' TOKEN_LABEL           {print_jump_zero_to_label($3);}
  | TOKEN_JUMP_ZERO  '!'  one_jump_operand     {print_jump_zero();}
  | TOKEN_JUMP_ZERO  '!' '!' two_jump_operands {print_jump_zero();}
  | TOKEN_JUMP_ZERO  not_empty_operand_sequence_form2  {print_jump_zero();}
  ;

jump_not_zero_instruction:
    TOKEN_JUMP_NOT_ZERO                            {print_jump_not_zero();}
  | TOKEN_JUMP_NOT_ZERO  '!' TOKEN_LABEL           {print_jump_not_zero_to_label($3);}
  | TOKEN_JUMP_NOT_ZERO  '!' one_jump_operand      {print_jump_not_zero();}
  | TOKEN_JUMP_NOT_ZERO  '!' '!' two_jump_operands {print_jump_not_zero();}
  | TOKEN_JUMP_NOT_ZERO  not_empty_operand_sequence_form2  {print_jump_not_zero();}
  ;

/*
call_instruction:
    TOKEN_CALL                            {print_call();}
  | TOKEN_CALL  '!' TOKEN_LABEL           {print_call_to_label($3);}
  | TOKEN_CALL  '!' one_jump_operand      {print_call();}
  | TOKEN_CALL  not_empty_operand_sequence_form2  {print_call();}
  ;
*/

/*
call_instruction:
    TOKEN_CALL                                      {print_call();}
  | TOKEN_CALL  '!' {print_start_call();}  operand  {print_end_call();}
  | TOKEN_CALL  not_empty_operand_sequence_form2    {print_call();}
  ;
*/

call_instruction:
    TOKEN_CALL                                      {print_call();}
  | TOKEN_CALL  '!' TOKEN_LABEL                     {print_call_to_label($3);}
  | TOKEN_CALL  '!' {print_start_call();}  one_call_operand  {print_end_call();}
  | TOKEN_CALL  not_empty_operand_sequence_form2    {print_call();}
  ;

one_jump_operand:
    num_operand
  | mnemonic_as_label_operand
  /* stack and expression may have extra offsets */
  | {op_depth++; op_1st_depth=op_depth;} stack_operand {op_depth--;}
  | {op_depth++; op_1st_depth=op_depth;} expr {op_depth--;}
  ;

/* Similar to one_jump_operand, but this first call operand is
   like a second operand, because the return value has been pushed already */
one_call_operand:
    num_operand
  | mnemonic_as_label_operand
  /* stack and expression may have extra offsets */
  | {op_depth++; op_1st_depth=op_depth+1;} stack_operand {op_depth--;}
  | {op_depth++; op_1st_depth=op_depth+1;} expr {op_depth--;}
  ;

one_non_numeric_operand:
    label_operand
  | mnemonic_as_label_operand
  /* stack and expression may have extra offsets */
  | {op_depth++; op_1st_depth=op_depth;} stack_operand {op_depth--;}
  | {op_depth++; op_1st_depth=op_depth;} expr {op_depth--;}
  ;

two_jump_operands:
    two_operands
  ;

two_operands:
    /* A form1 list of two operands */ /* generic operand may have extra offsets */
    {op_depth++;} {op_depth++; op_1st_depth=op_depth;} operand {op_depth--;} operand {op_depth--;}
  ;

operand_sequence:
    {/* empty operand sequence */}
  | not_empty_operand_sequence_form1
  | not_empty_operand_sequence_form2
  ;

/* push!!! 0 0 0 */
not_empty_operand_sequence_form1:
    '!'  { op_depth++; op_1st_depth=op_depth; } operand { op_depth--;}
  | '!'  { op_depth++;} not_empty_operand_sequence_form1  operand  {op_depth--;}
  ;

/* push *[0 0 ...] */
not_empty_operand_sequence_form2:
    '*' '['  {op_1st_depth=0;} operand_list ']' {op_list_depth = 0;}
  ;

operand_list:
    operand {op_list_depth++;}
  | operand_list  operand {op_list_depth++;}
  ;

operand:
    simple_operand
  | stack_operand
  | mnemonic_as_label_operand
  | expr
  ;

simple_operand:
    num_operand
  | label_operand
  ;

label_operand:
    TOKEN_LABEL          { $$=$1; print_token_label($1);}
  ;

num_operand:
    TOKEN_NUM            { $$=$1; print_token_num($1);}
  ;


/*It is more efficient to handle constant stack operand
  separately*/
stack_operand:
    TOKEN_STACK_OP_ADDR  { print_token_stack_op_addr($1); }
  | TOKEN_STACK_OP_VALUE { print_token_stack_op_value($1);}
  ;

/* This will push a label which is a mnemonic */
mnemonic_as_label_operand:
    mnemonic_as_label {print_token_label($1);}
  ;

/* To recognize a label which is a mnemonic */
mnemonic_as_label:
    TOKEN_PUSH   {$$="push";}
  | TOKEN_SET_SP {$$="set_sp";}
  | TOKEN_JUMP   {$$="jump";}
  | TOKEN_JUMP_ZERO  {$$="jump_zero";}
  | TOKEN_JUMP_NOT_ZERO  {$$="jump_not_zero";}
  | TOKEN_CALL   {$$="call";}
  | TOKEN_RETURN {$$="return";}
  | TOKEN_LOAD8  {$$="load8";}
  | TOKEN_LOAD4  {$$="load4";}
  | TOKEN_LOAD2  {$$="load2";}
  | TOKEN_LOAD1  {$$="load1";}
  | TOKEN_EXIT   {$$="exit";}
  | TOKEN_STORE8 {$$="store8";}
  | TOKEN_STORE4 {$$="store4";}
  | TOKEN_STORE2 {$$="store2";}
  | TOKEN_STORE1 {$$="store1";}
  | TOKEN_SIGX8  {$$="sigx8";}
  | TOKEN_SIGX4  {$$="sigx4";}
  | TOKEN_SIGX2  {$$="sigx2";}
  | TOKEN_SIGX1  {$$="sigx1";}
  | TOKEN_ADD    {$$="add";}
  | TOKEN_SUB    {$$="sub";}
  | TOKEN_MULT   {$$="mult";}
  | TOKEN_NEG    {$$="neg";}
  | TOKEN_AND    {$$="and";}
  | TOKEN_OR     {$$="or";}
  | TOKEN_XOR    {$$="xor";}
  | TOKEN_NOT    {$$="not";}
  | TOKEN_POW2   {$$="pow2";}
  | TOKEN_CHECK_VERSION   {$$="check_version";}
  | TOKEN_SHIFT_L  {$$="shift_l";}
  | TOKEN_SHIFT_RU {$$="shift_ru";}
  | TOKEN_SHIFT_RS {$$="shift_rs";}
  | TOKEN_DIV_U   {$$="div_u";}
  | TOKEN_DIV_S   {$$="div_s";}
  | TOKEN_REM_U   {$$="rem_u";}
  | TOKEN_REM_S   {$$="rem_s";}
  | TOKEN_LT_U    {$$="lt_u";}
  | TOKEN_LT_S    {$$="lt_s";}
  | TOKEN_LTE_U   {$$="lte_u";}
  | TOKEN_LTE_S   {$$="lte_s";}
  | TOKEN_EQ      {$$="eq";}
  | TOKEN_GTE_U   {$$="gte_u";}
  | TOKEN_GTE_S   {$$="gte_s";}
  | TOKEN_GT_U    {$$="gt_u";}
  | TOKEN_GT_S    {$$="gt_s";}
  | TOKEN_READ_FRAME {$$="read_frame";}
  | TOKEN_READ_PIXEL {$$="read_pixel";}
  | TOKEN_READ_CHAR  {$$="read_char";}
  | TOKEN_PUT_CHAR   {$$="put_char";}
  | TOKEN_PUT_BYTE   {$$="put_byte";}
  | TOKEN_NEW_FRAME  {$$="new_frame";}
  | TOKEN_SET_PIXEL  {$$="set_pixel";}
  | TOKEN_ADD_SAMPLE {$$="add_sample";}
  /* directives */
  | TOKEN_DATA8  {$$="data8";}
  | TOKEN_DATA4  {$$="data4";}
  | TOKEN_DATA2  {$$="data2";}
  | TOKEN_DATA1  {$$="data1";}
  | TOKEN_SPACE  {$$="space";}
  | TOKEN_EXPORT {$$="EXPORT";}
  | TOKEN_IMPORT {$$="IMPORT";}
  /* DO NOT FORGET to add this list to label_is_mnemonic() in ivm64_helper.c */
  ;

expr:
    {if (indata) start_complex_data();} signed_expr {if (indata) stop_complex_data();}
  ;

signed_expr:
    prefix_expr
  | '-' signed_expr {print_neg();}
  | '~' signed_expr {print_not();}
  /* Multiple '-' '~' before simple operands
     are handled as expressions -> this may be
     very inefficient, for example for constant
     aliases */
  | '-' simple_operand {print_neg();}
  | '~' simple_operand {print_not();}
  | '-' mnemonic_as_label_operand {print_neg();} /* simple_operand does not include mnemonic_as_label_operand */
  | '~' mnemonic_as_label_operand {print_not();}
  | '-' stack_operand {print_neg();}
  | '~' stack_operand {print_not();}
  ;

/* prefix expression */
prefix_expr:
    complex_stack_operand
  | prefix_binary_expr
  | prefix_unary_expr
  | prefix_zero_expr
  ;

/* Used when token after $ or & is a complex operand:
       push! $alias
       load8! $(load8 label)
       push! $$1
       push! &&0
       ...
   Increase op_expr_depth, as getsp pushes a word */
complex_stack_operand:
    '&' {fprintf(out_file, "getsp\n");} {op_expr_depth++;} operand {op_expr_depth--;} {print_token_stack_offset();}
  | '$' {fprintf(out_file, "getsp\n");} {op_expr_depth++;} operand {op_expr_depth--;} {print_token_stack_offset();} {fprintf(out_file, "load8\n");}
  ;

prefix_binary_expr:
    '(' {paren_expr_depth++;} '+' operand {op_expr_depth++;} operand  ')' {close_bin_expr("add");}
  | '(' {paren_expr_depth++;} '*' operand {op_expr_depth++;} operand  ')' {close_bin_expr("mult");}
  | '(' {paren_expr_depth++;} '&' operand {op_expr_depth++;} operand  ')' {close_bin_expr("and");}
  | '(' {paren_expr_depth++;} '|' operand {op_expr_depth++;} operand  ')' {close_bin_expr("or");}
  | '(' {paren_expr_depth++;} '^' operand {op_expr_depth++;} operand  ')' {close_bin_expr("xor");}
  | '(' {paren_expr_depth++;} '=' operand {op_expr_depth++;} operand  ')' {close_bin_expr(""); print_eq();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_LT_U     operand {op_expr_depth++;} operand ')' {close_bin_expr("lt");}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_LT_S     operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_lt_s();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_LTE_U    operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_lte_u();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_LTE_S    operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_lte_s();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_GT_U     operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_gt_u();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_GT_S     operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_gt_s();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_GTE_U    operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_gte_u();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_GTE_S    operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_gte_s();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_SHIFT_L  operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_shift_l();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_SHIFT_RU operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_shift_ru();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_SHIFT_RS operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_shift_rs();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_DIV_U    operand {op_expr_depth++;} operand ')' {close_bin_expr("div");}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_DIV_S    operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_div_s();}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_REM_U    operand {op_expr_depth++;} operand ')' {close_bin_expr("rem");}
  | '(' {paren_expr_depth++;} TOKEN_EXPR_REM_S    operand {op_expr_depth++;} operand ')' {close_bin_expr(""); print_rem_s();}
  ;

prefix_unary_expr:
    '(' {paren_expr_depth++;} TOKEN_LOAD8 operand ')' {close_unary_expr("load8");}
  | '(' {paren_expr_depth++;} TOKEN_LOAD4 operand ')' {close_unary_expr("load4");}
  | '(' {paren_expr_depth++;} TOKEN_LOAD2 operand ')' {close_unary_expr("load2");}
  | '(' {paren_expr_depth++;} TOKEN_LOAD1 operand ')' {close_unary_expr("load1");}
  | '(' {paren_expr_depth++;} TOKEN_SIGX8 operand ')' {close_unary_expr(""); print_sigx(8);}
  | '(' {paren_expr_depth++;} TOKEN_SIGX4 operand ')' {close_unary_expr(""); print_sigx(4);}
  | '(' {paren_expr_depth++;} TOKEN_SIGX2 operand ')' {close_unary_expr(""); print_sigx(2);}
  | '(' {paren_expr_depth++;} TOKEN_SIGX1 operand ')' {close_unary_expr(""); print_sigx(1);}
  ;

prefix_zero_expr:
  '(' {paren_expr_depth++;} TOKEN_READ_CHAR ')' {close_unary_expr("readchar");}
  ;

directive:
    data
  | space
  | export
  | import
  ;

data:
    open_data '[' num_operand  ']' { indata = 0; }
  | open_data '[' data_sequence_not_num ']' { indata = 0; }
  | open_data '[' num_operand data_sequence ']' { indata = 0; }
  /* Data repeticion is limited to one integer, e.g. "data8 [3]*5";
     statements like "data8 [3 0]*4" are not allowed yet*/
  | open_data '[' num_operand  ']' '*' TOKEN_NUM
        {print_num_data_repetition($3, $6-1); /*Note that at least one data is printed, even if the repetition is 0 or negative*/ }
        {indata = 0; }
  ;

open_data:
    open_data8
  | open_data4
  | open_data2
  | open_data1
  ;

open_data8: TOKEN_DATA8 {indata = 8;} ;
open_data4: TOKEN_DATA4 {indata = 4;} ;
open_data2: TOKEN_DATA2 {indata = 2;} ;
open_data1: TOKEN_DATA1 {indata = 1;} ;

data_sequence:
    allowed_data_operand
  | allowed_data_operand data_sequence
  ;

allowed_data_operand:
    simple_operand
  | mnemonic_as_label_operand
  | expr
  ;

/* A data sequence not starting with a number */
data_sequence_not_num:
    allowed_data_operand_not_num
  | allowed_data_operand_not_num data_sequence
  ;

allowed_data_operand_not_num:
    label_operand
  | mnemonic_as_label_operand
  | expr
  ;

/*
space_old:
    TOKEN_SPACE TOKEN_NUM {print_space($2);}
  | TOKEN_SPACE allowed_data_operand_not_num {fprintf(stderr, "[as0] Error: Only 'space <numeric constant>' is supported\n");
                                              fprintf(out_file, "[as0] Error\n"); // This makes next stage fail
                                              exit(EXIT_FAILURE);}
  ;
*/

space:
    TOKEN_SPACE TOKEN_NUM {print_space_num($2);}
  | TOKEN_SPACE {indata = 8;} allowed_data_operand_not_num {indata = 0; print_space_expr(cdata); /*The last complex data is the space amount in bytes*/ }
  ;

export:
    TOKEN_EXPORT export_operand {/*Not meaningful for now*/}
  ;

export_operand:
    TOKEN_LABEL
  | mnemonic_as_label_operand
  ;

import:
    TOKEN_IMPORT {fprintf(stderr, "[as0] Error: IMPORT clause not supported\n");
                  fprintf(out_file, "[as0] Error\n"); /* This makes next stage fail */
                  exit(EXIT_FAILURE);}
  ;


alias:
    /* if label is a native mnemonic for as1, prepend '%' char */
    TOKEN_LABEL '=' TOKEN_LABEL {fprintf(out_file, "%s%s=%s%s #as0 alias\n",
                                                   MNEMONIC_LABEL_PREFIX($1), $1, MNEMONIC_LABEL_PREFIX($3), $3);}
  | TOKEN_LABEL '=' TOKEN_NUM   {fprintf(out_file, "%s%s=%ld #as0 alias\n", MNEMONIC_LABEL_PREFIX($1), $1, $3);}
  | TOKEN_LABEL '=' mnemonic_as_label        {fprintf(out_file, "%s%s=%%%s #as0 alias\n", MNEMONIC_LABEL_PREFIX($1), $1, $3);}
    /* when label is a mnemonic, prepend '%' char */
  | mnemonic_as_label '=' TOKEN_LABEL        {fprintf(out_file, "%%%s=%s%s #as0 alias\n", $1, MNEMONIC_LABEL_PREFIX($3), $3);}
  | mnemonic_as_label '=' TOKEN_NUM          {fprintf(out_file, "%%%s=%ld #as0 alias\n", $1, $3);}
  | mnemonic_as_label '=' mnemonic_as_label  {fprintf(out_file, "%%%s=%%%s #as0 alias\n", $1, $3);}
  | alias_decl_not_supported {
                            fprintf(stderr,
                            "[as0] Error: only label and simple numeric aliases are supported "
                            "(not expressions, ...)\n(line %d '%s=...')\n"
                            "Hint: use preprocessor before assembling (-p)\n\n", yylineno, $1);
                            fprintf(out_file, "[as0] Error\n"); /* This makes next stage fail */
                            exit(EXIT_FAILURE);}
  ;

/* Only label1=label2 or label=integer
   are supported aliases.
   The rsh cannot be stack operands nor expressions ...
*/
alias_decl_not_supported:
    TOKEN_LABEL '=' alias_rhs_not_supported {$$=$1;}
  | mnemonic_as_label '=' alias_rhs_not_supported {$$=$1;}
  ;

alias_rhs_not_supported:
    stack_operand
  | expr
  ;

%%

/* Remove initialization file, to be used with atexit() */
void remove_files(void){
    if (init_filename){
        remove(init_filename);
    }
}

int main(int argc, char *argv[]){

    fprintf(stderr, "IVM64 flex/bison-based asm to bin %s - compatible with %s\n", IVM64_BISON_AS_VERSION, IVM64_ARCHITECTURE);

    out_file = stdout;

    /* Options:
         -e entry_point
    */
    int c;
    char *entry_point = NULL;
    while ((c = getopt (argc, argv, "e:v")) != -1) {
        switch (c) {
          case 'e':
            if (optarg) {
                entry_point = strdup(optarg);
                fprintf(out_file, "#ENTRY POINT = %s\n", entry_point);
            }
            break;
          case 'v':
            /* Show version and quit */
            exit(EXIT_SUCCESS);
            break;
          default:
            fprintf(stderr, "Usage: %s [-e entry_point]\n", argv[0]);
            exit(EXIT_FAILURE);
         }
    }

    fprintf(out_file, ".LIVM64_bison_first_position:\n");
    fprintf(out_file, "#call! .LIVM64_bison_init_code #:\n");
    /* No entry point yet, using the first instruction of the
       program */
    fprintf(out_file, "  push8 .LIVM64_bison_program_start\n");
    fprintf(out_file, "  push8 .LIVM64_bison_init_code\n");
    fprintf(out_file, "  jump\n\n");

    /* Before the program itself, two words are added with
       execution time information */
    fprintf(out_file, ".LIVM64_bison_info_arg_start:\n"
                      "   data8 [0]\n"
                      ".LIVM64_bison_info_heap_start:\n"
                      "   data8 [0]\n");
    fprintf(out_file, "\n");

    /* The program itself starts here */
    fprintf(out_file, "# Here the program STARTS\n");
    fprintf(out_file, ".LIVM64_bison_program_start:\n");

    /* Support for "entry point"
       A call to the entry point label, passing the
       3 arguments according to the ivm entry point
       convention*/
    /*
       With entry point:
    0x0 +-----------------+ <- .LIVM64_bison_first_position
        |      ...        |
        | #call to the    |
        | #initialization |
        | #code           |
        |      ...        |
        | push! heapStart |
        | push! argLength |
        | push! argStart+8|
        | call! _start    |
        |      ...        |
        | exit            | <- .LIVM64_bison_entry_point_exit
        +-----------------+
        | argStart        | <- .LIVM64_bison_info_arg_start
        | heapStart       | <- .LIVM64_bison_info_heap_start
        +-----------------+
        | #program        | <- progStart  = .LIVM64_bison_program_start
        | __start:        |
        |                 |
        | _start:         | <- entry point
        |                 |
        |      ...        |
        |                 |
        | #initialization |
        | #code is placed |
        | #here after the |
        | #program        |
        +-----------------+
        | argLength       | <- argStart = .LIVM64_bison_last_position
        |                 | <- argStart+8
        |      ...        |
        +-----------------+
        |                 | <- space
        |      ...        |
        +-----------------+
        |                 | <- heapStart
        |      ...        |
        +-------------
    */
    if (entry_point){
        // If the entry point label is a mnemonic, add "%" prefix
        char *prefix = label_is_mnemonic(entry_point)?"%":"";
        fprintf(out_file, "# Entry point call \n"
                          "push8 .LIVM64_bison_info_heap_start\n"
                          "load8\n"
                          "push8 .LIVM64_bison_last_position\n"
                          "load8\n"
                          "push8 .LIVM64_bison_last_position\n"
                          "push1 8\n"
                          "add\n"
                          "push8 .LIVM64_bison_entry_point_exit\n"
                          "push8 %s%s\n"
                          "jump\n"
                          ".LIVM64_bison_entry_point_exit:\n"
                          "getsp\n"
                          "push1 16\n"
                          "add\n"
                          "setsp\n"
                          "exit\n\n", prefix, entry_point);
    }

    /* Preamble of the initialization file */

    /* Generate a unique file name */
    init_filename = gen_tempfile(INITIALIZATION_FILE);

    init_file = fopen(init_filename, "w");
    if (!init_file) {
        fprintf(stderr, "[as0] Error opening %s\n", init_filename);
        fprintf(out_file, "[as0] Error\n"); /* This makes next stage fail */
        exit(EXIT_FAILURE);
    }

    atexit(remove_files);

    fprintf(init_file, "\n# Initialization code STARTS\n");
    fprintf(init_file, ".LIVM64_bison_init_code:\n");

    /* --- PARSING ---- */
    int r = yyparse();   // This writes to out_file and init_file
    if (! noerror){ /* Error in lexer */
        remove(init_filename);
        fprintf(stderr, "[as0] Error: a lexer error was found ... as0 exiting\n");
        fprintf(out_file, "[as0] Error\n"); /* This makes next stage as1 fail */
        exit(EXIT_FAILURE);
    }
    /* ---------------- */
    /* Print an exit insn in the case the assembly is an empty file */
    fprintf(out_file, "# Here the program ENDS\n");
    fprintf(out_file, ".LIVM64_bison_program_ended:\n");
    fprintf(out_file, "exit\n");

    /* Let's include in the initialization file the
       execution-time information: the start of the arg file
       and the heap*/
    /*
    With NO entry point:
    +-----------------+ 0x0   <- .LIVM64_bison_first_position
    |      ...        |
    | #call to the    |
    | #initialization |
    | #code           |
    |      ...        |
    |                 |
    |                 |
    | jump! progStart |
    |      ...        |
    +-----------------+
    | argStart        | <- .LIVM64_bison_info_arg_start
    | heapStart       | <- .LIVM64_bison_info_heap_start
    +-----------------+
    | #program        | <- progStart = .LIVM64_bison_program_start
    | __start:        |
    |                 |
    |      ...        |
    |                 |
    | #initialization |
    | #code is placed |
    | #here after the |
    | #program        |
    +-----------------+
    | argLenght       | <- argStart = .LIVM64_bison_last_position
    |                 |
    |      ...        |
    +-----------------+
    |                 | <- space
    |      ...        |
    +-----------------+
    |                 | <- heapStart
    |      ...        |
    */
    /* The argument file starts just where the program ends*/
    fprintf(init_file, "# Run time information: arg. start\n"
                       "push8 .LIVM64_bison_last_position\n"
                       "push8 .LIVM64_bison_info_arg_start\n"
                       "store8\n\n"
    );
    /*The first position of the argument file has the argument
      length*/
    /* The heap starts at the lenght of the argument file
       after the program ends, adding a word for the length,
       plus the space area
    */
    fprintf(init_file, "# Run time information: heap start\n"
                        "push8 .LIVM64_bison_last_position\n"
                        /* Arg file length + 8 */
                        "push8 .LIVM64_bison_last_position\n"
                        "load8\n"
                        "add\n"
                        "push1 8\n"
                        "add\n"
                        /* Space area */
                        /* .LIVM64_bison_space_bytes been user as byte count for the area space (updated in each space declaratio) */
                        "push8 .LIVM64_bison_space_bytes\n"
                        "load8\n"
                        "add\n"
                        /* Write the heap start information */
                        "push8 .LIVM64_bison_info_heap_start\n"
                        "store8\n\n"
     );

//~    fprintf(init_file, "# Run time information: heap start\n"
//~                       "push8 .LIVM64_bison_last_position\n"
//~                       /* Arg file length + 8 */
//~                       "push8 .LIVM64_bison_last_position\n"
//~                       "load8\n"
//~                       "add\n"
//~                       "push1 8\n"
//~                       "add\n"
//~                       /* Space area */
//~                       "push8 %ld\n"
//~                       "add\n"
//~                       /* Write the heap start information */
//~                       "push8 .LIVM64_bison_info_heap_start\n"
//~                       "store8\n\n", space
//~    );

    /* Return to the call to inilialize*/
    fprintf(init_file, "jump # return\n");

    fprintf(init_file, "# Initialization extra data\n\n");
    fprintf(init_file, ".LIVM64_bison_space_bytes:\n"); /* Counts the bytes allocated in the space area so far*/
    fprintf(init_file, "               data8 [8]\n");   /* At least 8 for storing the length of this area at the beginning of the space area */


    fprintf(init_file, "# Initialization code ENDS\n\n");
    fclose(init_file);

    file_put_contents(init_filename, out_file);
    fprintf(out_file, ".LIVM64_bison_last_position:\n\n");
    remove(init_filename);

    return r;
}
