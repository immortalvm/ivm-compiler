/*
 * Preservation Virtual Machine Project
 * Helper functions for the ivm64 target
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: Oct 2019 - Nov 2021
 *
 */

/* Define IN_TARGET_CODE before including config.h */
#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "c-family/c-common.h" // Provides cpp_define
#include "stringpool.h"
#include "attribs.h"
#include "langhooks.h"
#include "df.h"
#include "tm.h"
#include "memmodel.h"
#include "tm_p.h"
#include "optabs.h"
#include "opts.h"
#include "regs.h"
#include "emit-rtl.h"
#include "calls.h"
#include "varasm.h"
#include "conditions.h"
#include "output.h"
#include "expr.h"
#include "reload.h"
#include "builtins.h"
#include "dbxout.h"
#include "varasm.h"
#include "function.h"
#include "tree-pass.h"
#include "context.h"
#include "pass_manager.h"
#include "input.h"
#include "recog.h"
#include "explow.h"
#include "insn-attr.h"
#include "diagnostic.h"
#include "flag-types.h" // REORDER_BLOCKS_ALGORITHM_STC
#include "tm_p.h"       // To use satisfies_constraint_X() include this after rtl.h
#include "tm-constrs.h" // To use satisfies_constraint_X()
#include "debug.h"
#include "print-rtl.h"  // Use debug_rtx(rtx) to print a rtl expression to sderr (see print-rtl.c)
                        // Use str_pattern_slim(rtx) to get a string with a pretty-printed rtx
#include "print-tree.h" // Provides debug_tree(tree)
#include "ivm64-protos.h"


#include <stdio.h>

/* This file should be included last.  */
#include "target-def.h"


// -----------------------------------------------------------------------------
// Usefull macros
// -----------------------------------------------------------------------------
#define RETURN_ON_ARGS (global_options.x_optimize > 0 \
                        && global_options.x_flag_tree_coalesce_vars \
                        && !ivm64_get_cfun_volatil_ret())


/* Extern symbols used in this file */
extern int reload_in_progress;
extern int reload_completed;
extern short *reg_renumber;

/* Local prototypes */
static void ivm64_output_indirect_push(rtx, enum machine_mode);
static void ivm64_output_indirect_pop(rtx, enum machine_mode);

/* Offset in bytes of the first general purpose register (AR)
   with respect to SP */
int ivm64_gpr_offset = 0;

/* Extra offset to be added to ivm64_gpr_offset to manage explicit
   push/pop operations */
int ivm64_stack_extra_offset = 0;

/* To keep account of the emitted push/pop, total and by the current function */
int emitted_push_cfun = 0;
int emitted_pop_cfun = 0;

/* Fast-pop at prolog */
int ivm64_prolog_fast_pop = 0;

/* Number of extra peephole2 passes*/
static int ivm64_extra_peep2_count = 0;

/* Ids of extra peephole2 passes*/
static int ivm64_extra_peep2_pass_number[8];

/* This variable contains a pointer to the operand used as
   destination in the last operand transfer */
rtx ivm64_last_set_operand = 0;

/* An alias for generating TR_REGNUM register rtx*/
#define TR_REG_RTX(mode) (gen_rtx_REG((mode), TR_REGNUM))

/* True if current function returns a volatile type */
static bool ivm64_get_cfun_volatil_ret();

/* Run-time Target Specification.  */
void ivm64_cpu_cpp_builtins(struct cpp_reader *pfile)
{
    #define builtin_assert(TXT) cpp_assert (pfile, TXT)
    #define builtin_define(TXT) cpp_define (pfile, TXT)
    builtin_define ("__ivm64__");
}

/* A C structure for machine-specific, per-function data.
   This is added to the cfun structure.  */
struct GTY(()) machine_function
{
    bool volatile_ret;
};

/* To enable/disable individual peephole patterns
   Return 1 if a peephole is enabled, 0 otherwise  */
#define IVM64_SET_PEEP(peep2, enable) case peep2: return enable; break;
int ivm64_peep_enabled(enum ivm64_peephole2 peep)
{
    switch(peep) {
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP,                        1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP_PUSH_POP,               1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_MOVE,                       1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH_POP_PUSH,               1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_MOVE,                        1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_MOVE2,                       1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSHARG,                     1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSHARG,                    1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_BINOP,                      1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP1,                          1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP_1_2_3,                     1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP_1_2,                       1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP_1_DEADREG_BINOP_2,         1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP_1_2_DEADREG_BINOP_3,       1);
        IVM64_SET_PEEP(IVM64_PEEP2_ZERO_EXTEND,                     1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP_DEADREG_BINOP,          1);
        IVM64_SET_PEEP(IVM64_PEEP2_UNARY_DEADREG_BINOP,             1);
        IVM64_SET_PEEP(IVM64_PEEP2_UNARY_1_BINOP_2_DEADREG_BINOP_3, 1);
        IVM64_SET_PEEP(IVM64_PEEP2_RDX_1_2,                         1);
        IVM64_SET_PEEP(IVM64_PEEP2_SIGNX,                           1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSH_COMMUTATIVE,           1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSH_IMM_COMMUTATIVE,       1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH_SUB,                    1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSH_SUB,                   1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH_COMMUTATIVE,            1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSH,                       1);
        IVM64_SET_PEEP(IVM64_PEEP2_POW2,                            1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE,                            1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH,                        1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSHDI,                      1);
        IVM64_SET_PEEP(IVM64_PEEP2_POPDI_PUSH,                      1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_IND_PUSH,                    1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_IND_OFFSET_PUSH,             1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_CBRANCH,                     1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_CBRANCH_REV,                 1);
        IVM64_SET_PEEP(IVM64_PEEP2_POPDI_CBRANCH,                   1);
        IVM64_SET_PEEP(IVM64_PEEP2_POPDI_CBRANCH_REV,               1);
        IVM64_SET_PEEP(IVM64_PEEP2_SET_CBRANCH,                     1);
        IVM64_SET_PEEP(IVM64_PEEP2_SET_CBRANCH_REV,                 1);
        IVM64_SET_PEEP(IVM64_PEEP2_SIGNX_POP_CBRANCH_REV,           1);
        IVM64_SET_PEEP(IVM64_PEEP2_SET_NOP,                         1);
        IVM64_SET_PEEP(IVM64_PEEP2_NOP_SET,                         1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_NOP_PUSH,                    1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOV_PUSH,                        1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOV_PUSH_POP_PUSH_POP,           1);
        IVM64_SET_PEEP(IVM64_PEEP2_CALL_PUSH_AR,                    1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_BINOP,                      1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP_PUSH_BINOP_MULTIMODE,   1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_INDPUSH_ZERO_EXTEND,        1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_SUB_NEG,                    1);
        IVM64_SET_PEEP(IVM64_PEEP2_AND_POP,                         1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_ADD_POP_PUSH_IND_POP,       1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_ADD_POP_PUSH_IND_OFFSET_POP,1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_MOVE_MOVE,                  1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH_BINOP_COMMUTATIVE,      1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH_BINOP_SUB,              1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP_CASESI,                 1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_SIGNX_POP_CASESI,           1);
        IVM64_SET_PEEP(IVM64_PEEP2_SIGNX_SIGNX,                     1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_SIGNX,                      1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP_PUSH_SIGNX,             1);
        IVM64_SET_PEEP(IVM64_PEEP2_INDPUSH_POP_PUSH_SIGNX,          1);
        // assembly peepholes
        IVM64_SET_PEEP(IVM64_PEEP1_MOVE_PUSH_POP,                   1);
        IVM64_SET_PEEP(IVM64_PEEP1_SHIFTRU63_AND,                   1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSHAR_BINOP_POPAR,              1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_COMMUTATIVE_POPAR,          1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_SUB_POPAR,                  1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_BINOP_BINOP_POPAR,          1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_BINOP_POPAR,                1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_ADD,                        1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSHAR_UNARY_POPAR,              1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSHAR_SIGNX_POPAR,              1);
        IVM64_SET_PEEP(IVM64_PEEP1_POP_PUSH,                        1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_SIGNX,                      1);
        IVM64_SET_PEEP(IVM64_PEEP1_IND_PUSH_SIGNX,                  1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_POPAR,                      1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_POPMEMAR,                   1);
    }
    return 0;
}


/* Return the ivm instruction mnemonic associated to a
   binary RTX operator*/
const char *ivm64_rtxop2insn(rtx op)
{
    return ivm64_rtxcode2insn(GET_CODE(op));
}

/* Return the ivm instruction mnemonic associated to a
   binary operation RTL code*/
const char *ivm64_rtxcode2insn(int code)
{
    const char *s;
    switch (code){
        case PLUS:
            s = "add"; break;
        case MINUS:
            s = "sub"; break;
        case MULT:
            s = "mult"; break;
        case DIV:
            s = "div_s"; break;
        case MOD:
            s = "rem_s"; break;
        case UDIV:
            s = "div_u"; break;
        case UMOD:
            s = "rem_u"; break;
        case AND:
            s = "and"; break;
        case IOR:
            s = "or"; break;
        case XOR:
            s = "xor"; break;
        case LSHIFTRT:
            s = "shift_ru"; break;
        case ASHIFTRT:
            s = "shift_rs"; break;
        case ASHIFT:
            s = "shift_l"; break;
        default:
            ivm64_fatal("Not supported binary operator");
    }
    return s;
}


/* Function to abort on error */
void ivm64_fatal(const char *msg)
{
    fprintf (stderr, "%s: Internal IVM64-GCC abort.\n%s\n", main_input_filename, msg);
    exit(FATAL_EXIT_CODE);
}

/* Determine if the current pass has the name "name" */
int ivm64_pass_in_progress(const char *name)
{
    if (current_pass == NULL)
         return false;
    return (0 == strcmp (current_pass->name, name));
}

/* Determine if the current pass is postreload */
int ivm64_postreload_in_progress()
{
    return ivm64_pass_in_progress("postreload");
}

/* Determine if the current pass is one of the extra peephole2 passes.
   For example, ivm64_extra_peep2_in_progress(2) is true if the
   current pass is "rtl-peephole2-extra2". The call
   ivm64_extra_peep2_in_progress(0) refers to the original "peephole2" pass. */
int ivm64_extra_peep2_in_progress(int extra)
{
    return ivm64_extra_peep2_pass_number[extra] == current_pass->static_pass_number;
}

/* Add an extra peephole2 pass after the pass with name 'passname'. Apparently
   only one can be added after standard passes. Remember that all passes can
   be listed with "gcc program.c -fdump-passes". */
static void ivm64_add_extra_peep2_pass(const char *passname)
{
    ivm64_extra_peep2_count++;

    char *new_passname = (char*)xmalloc(1024);
    sprintf(new_passname, "rtl-peephole2-extra%d", ivm64_extra_peep2_count);

    opt_pass *peephole2_extra = g->get_passes ()->get_pass_by_name("rtl-peephole2")->clone ();
    struct register_pass_info peep2_extra_info = {peephole2_extra, passname, 1, PASS_POS_INSERT_AFTER };
    g->get_passes()->register_pass_name (peephole2_extra, new_passname);
    register_pass (&peep2_extra_info);
    ivm64_extra_peep2_pass_number[ivm64_extra_peep2_count] = peephole2_extra->static_pass_number;

    free(new_passname);
}


/* We are interested in getting the register used by the current function with
   the highest number. This function returns it. */
static int calc_live_max_reg()
{
    int maxreg = 0;
    for (int reg = 0; reg < FIRST_PSEUDO_REGISTER; reg++) {
        if (df_regs_ever_live_p (reg)) {
            if (maxreg < reg) maxreg = reg;
        }
    }
    return maxreg;
}

/* Return the number of GPRs used by the current function */
static int used_gp_regs(){
    int maxreg = calc_live_max_reg();
    maxreg = maxreg - FIRST_GP_REGNUM +1 ;
    if (cfun->calls_setjmp){
        return NUM_GP_REGISTERS;
    }
    if (global_options.x_flag_unroll_loops) {
        return (maxreg>2)?maxreg:2;
    }
    return (maxreg>0)?maxreg:0;
}

/* Return true if an RTL pattern corresponding to a call
   to label was inserted while passing arguments in the stack.
   Typically this happens when passing structures. In these
   cases, gcc inserts calls to 'memcpy' or 'memset' when some
   other arguments has been already pushed. We need to prevent
   these pushed arguments from being overwriten by the return
   value of these calls inserted by gcc. */
int is_a_push_memcpy(rtx nbytes){
    /* If a call was inserted while passing arguments
       the stack is not balanced at this moment*/
    return !!(ivm64_gpr_offset - XUINT(nbytes, 0));
}

/*  Return true if a call insn is annotated with REG_NORETURN.
    These functions can lead to stack unbalance if arguments are
    not popped after the call. */
int is_noreturn(rtx call_insn){
    if (find_reg_note (call_insn, REG_NORETURN, NULL_RTX) != NULL_RTX) {
        return 1;
    }
    return 0;
}

/* Return the REG_ARGS_SIZE annotation value. */
bool ivm64_reg_args_size(rtx pusharg_insn, HOST_WIDE_INT *reg_args_size){
    HOST_WIDE_INT ret;
    rtx note = find_reg_note (pusharg_insn, REG_ARGS_SIZE, NULL_RTX);
    if (note != NULL_RTX && CONSTANT_P(XEXP(note,0))) {
        *reg_args_size = INTVAL(XEXP(note,0));
        return true;
    }
    return false;
}

/* Return true if the instruction has an REG_ARGS_SIZE note
   and its value is 0. Otherwise (missing note or not zero)
   return false.
*/
HOST_WIDE_INT ivm64_reg_args_size_is_zero(rtx insn){
    HOST_WIDE_INT reg_args_size;
    int note = ivm64_reg_args_size(insn, &reg_args_size);
    if (note && reg_args_size == 0)
        return true;
    else
        return false;
}

/* In a call instruction, return if the return value (AR) is
   unused*/
int return_value_unused(rtx call_insn){
    if (find_regno_note (call_insn, REG_UNUSED, AR_REGNUM) != NULL_RTX
       && !CALL_INSN_FUNCTION_USAGE(call_insn)) {
        return 1;
    }
    return 0;
}

/* If insn has a note to be equivalent to a constant
   (integer, label, ref ...), return the constant rtx,
   else NULL_RTX*/
rtx const_reg_equiv(rtx insn){
    rtx note = find_reg_note(insn, REG_EQUIV, NULL_RTX);
    if (note != NULL_RTX){
        if (CONSTANT_P(XEXP(note,0))){
            return XEXP(note,0);
        }
    }
    return NULL_RTX;
}

/* Function copy_rtx() does not duplicate REGs as they are
   considered shareable (see emit-rtl.c:copy_rtx())
   This helper function solves this */
rtx ivm64_copy_rtx(rtx orig){
  if (REG_P(orig)) {
    return gen_rtx_REG(GET_MODE(orig), REGNO(orig));
  } else {
    return copy_rtx(orig);
  }
}


/* EXPAND FUNCTIONS */

/* Push one operand on the stack. */
void ivm64_expand_push(rtx operand, enum machine_mode mode)
{
    rtx rtx_unspec_push;
    rtx rtx_set;

    if (!tr_register_operand(operand, mode)){
        rtx rtx_tr_reg = gen_rtx_REG(mode, TR_REGNUM);
        rtx_unspec_push = gen_rtx_UNSPEC_VOLATILE(mode,
                                        gen_rtvec (2, rtx_tr_reg, operand),
                                        UNSPEC_PUSH_TR);
        rtx_set = gen_rtx_SET(
                    rtx_tr_reg,
                    rtx_unspec_push);
        emit_insn(rtx_set);
    } else {
        rtx rtx_tr_reg_di = gen_rtx_REG(DImode, TR_REGNUM);
        rtx_unspec_push = gen_rtx_UNSPEC_VOLATILE(DImode,
                                        gen_rtvec (1, rtx_tr_reg_di),
                                        UNSPEC_PUSH_TR_TR);
        rtx_set = gen_rtx_SET(
                    rtx_tr_reg_di,
                    rtx_unspec_push);
        emit_insn(rtx_set);
    }
}

/* Pop one operand from the stack. */
void ivm64_expand_pop(rtx operand, enum machine_mode mode)
{
    rtx rtx_unspec_pop;
    rtx rtx_set;

    if (!tr_register_operand(operand, mode)){
        rtx rtx_tr_reg = gen_rtx_REG(mode, TR_REGNUM);
        rtx_unspec_pop = gen_rtx_UNSPEC_VOLATILE(mode,
                                        gen_rtvec (1, rtx_tr_reg),
                                        UNSPEC_POP_TR);
        rtx_set = gen_rtx_SET(operand, rtx_unspec_pop);
        emit_insn(rtx_set);
        emit_insn(gen_rtx_CLOBBER(VOIDmode, TR_REG_RTX(mode)));
    } else {
        rtx rtx_tr_reg_di = gen_rtx_REG(DImode, TR_REGNUM);
        rtx_unspec_pop = gen_rtx_UNSPEC_VOLATILE(DImode,
                                        gen_rtvec (1, rtx_tr_reg_di),
                                        UNSPEC_POP_TR_TR);
        rtx_set = gen_rtx_SET(
                    rtx_tr_reg_di,
                    rtx_unspec_pop);
        emit_insn(rtx_set);
    }

    ivm64_last_set_operand = operand;
}

/* Emit a sequence of RTL instructions to move one source to a
   destination operand, by pushing the source and popping the destination.
   This function can be called also when pushing function arguments
   of size greater than one word*/
int ivm64_expand_move(rtx *operands, enum machine_mode mode)
{
    /* Normal move; not pushing an argument */
    if (! push_operand (operands[0], mode))
    {
        ivm64_expand_push (operands[1], mode);
        if (gp_register_operand (operands[0], mode)){
            /* If destination is a GPR, clobber it before pop */
            emit_insn(gen_rtx_CLOBBER(VOIDmode,
                                      gen_rtx_REG(mode, REGNO(operands[0]))));
        }
        ivm64_expand_pop (operands[0], mode);
        return 1;
    }

    /* If this point is reached, a function argument is pushed, so let the
       pattern `pushm1' solve it. This may happen for example when pushing
       a complex double that it is decomposed on two one-word sized pushes
       (real and imaginary parts) */
    emit_insn (gen_rtx_CLOBBER( VOIDmode, TR_REG_RTX (mode)));
    return 0;
}


/* In commutative operations try to place the simplest operand
   first, specially if it was the last set operand. In this way
   we help some of the peephole optimizations. */
static int ivm64_swap_operands (rtx *op1, rtx *op2)
{
    rtx tmp;
    if (! pushable_operand(*op2, VOIDmode)
        || (pushable_operand(*op1, VOIDmode)
            && REG_P(*op2) && ivm64_last_set_operand
            && REG_P(ivm64_last_set_operand)
            && (REGNO(*op2) == REG_P(ivm64_last_set_operand)))
        || (REG_P(*op2) && (REGNO(*op2) == STACK_POINTER_REGNUM))
        )
    {
        tmp = *op1;
        *op1 = *op2;
        *op2 = tmp;
        return 1;
    }
    return 0;
}

/* Emit a sequence of RTL instructions for an arithmetic binary operation.
   Set commutative to true for commutative operators. */
static int ivm64_expand_binary(rtx *operands, enum machine_mode mode,
                            enum rtx_code code, bool commutative)
{
    if (commutative)
        ivm64_swap_operands(&operands[1], &operands[2]);
    if (! arithmetic_operand(operands[2], mode))
        operands[2] = force_reg (mode, operands[2]);
    ivm64_expand_push(operands[1], mode);
    emit_insn(gen_rtx_SET(TR_REG_RTX(mode),
                          gen_rtx_fmt_ee(code, mode,
                                         TR_REG_RTX(mode),
                                         operands[2])));
    ivm64_expand_pop(operands[0], mode);
    return 1;
}


/* Emit a sequence of RTL instructions to add two operands.
   Do not forget that the increasing of SP is handled by a
   separate RTL pattern (zero returned for this case). */
int ivm64_expand_add(rtx *operands, enum machine_mode mode)
{
    if (! sp_register_operand(operands[0], DImode)) {
        return ivm64_expand_binary(operands, mode, PLUS, 1);
    }

    /* Remember: add to SP is handled separately to the general sum*/
    return 0;
}

/* Emit a sequence of RTL instructions for commutative operations
   other than add (i.e.: code in {MULT, AND, IOR, XOR). */
int ivm64_expand_commutative(rtx *operands, enum machine_mode mode,
                             enum rtx_code code)
{
    return ivm64_expand_binary(operands, mode, code, 1);
}

/* Emit a sequence of RTL instructions for non-commutative operations,
   i.e.: code in {SUB, DIV, MOD, UDIV, UMOD, ASHIFT, ASHIFTRT, LSHIFTRT). */
int ivm64_expand_non_commutative(rtx *operands, enum machine_mode mode,
                                 enum rtx_code code)
{
    return ivm64_expand_binary(operands, mode, code, 0);
}

/* Emit a sequence of RTL instructions for an unary arithmetic operation. */
int ivm64_expand_unary(rtx *operands, enum machine_mode srcmode,
                            enum machine_mode dstmode,
                            enum rtx_code code)
{
    ivm64_expand_push(operands[1], srcmode);
    emit_insn(gen_rtx_SET(TR_REG_RTX(dstmode),
                          gen_rtx_fmt_e(code, dstmode,
                                        TR_REG_RTX(srcmode))));
    ivm64_expand_pop(operands[0], dstmode);
    return 1;
}

/* Emit a sequence of RTL instructions for pattern 'call' */
void ivm64_expand_call(rtx *operands)
{
    rtx rtx_fnaddr, rtx_callarg, rtx_call;
    rtx_fnaddr = operands[0];
    rtx_callarg = operands[1];

    if (! CONSTANT_ADDRESS_P(XEXP(rtx_fnaddr, 0))) {
        rtx new_operands[2];
        new_operands[0] =  gen_rtx_REG (DImode, AR_REGNUM);
        new_operands[1] =  XEXP(rtx_fnaddr, 0);
        ivm64_expand_move(new_operands, DImode);

        rtx_fnaddr = gen_rtx_MEM(DImode, gen_rtx_REG (DImode, AR_REGNUM));
    }

    rtx_call = gen_rtx_CALL (VOIDmode, rtx_fnaddr, rtx_callarg);
    emit_call_insn (rtx_call);
}

/* Emit a sequence of RTL instructions for pattern 'call_value' */
void ivm64_expand_call_value (rtx *operands)
{
    rtx rtx_retval, rtx_fnaddr, rtx_callarg, rtx_set, rtx_call;
    rtx_retval = operands[0];
    rtx_fnaddr = operands[1];
    rtx_callarg = operands[2];

    if (! CONSTANT_ADDRESS_P(XEXP(rtx_fnaddr, 0))) {
        rtx new_operands[2];
        new_operands[0] =  gen_rtx_REG (DImode, AR_REGNUM);
        new_operands[1] =  XEXP(rtx_fnaddr, 0);
        ivm64_expand_move(new_operands, DImode);

        rtx_fnaddr = gen_rtx_MEM(DImode, gen_rtx_REG (DImode, AR_REGNUM));
    }

    rtx_call = gen_rtx_CALL (VOIDmode, rtx_fnaddr, rtx_callarg);
    rtx_set = gen_rtx_SET (rtx_retval, rtx_call);

    emit_call_insn (rtx_set);
}

/* Emit a sequence of RTL instructions for pattern 'call_pop' */
void ivm64_expand_call_pop (rtx *operands)
{

    rtx rtx_fnaddr, rtx_callarg1;
    rtx rtx_reg_SP, rtx_N, rtx_SP_plus_N, rtx_pop, rtx_call;

    rtx_fnaddr   = operands[0];
    rtx_callarg1 = operands[1];
    rtx_N        = operands[3];

    rtx_call = gen_rtx_CALL(VOIDmode, rtx_fnaddr, rtx_callarg1);
    rtx_reg_SP = gen_rtx_REG(DImode, STACK_POINTER_REGNUM);
    rtx_SP_plus_N = gen_rtx_PLUS(Pmode, rtx_reg_SP, rtx_N);
    rtx_pop = gen_rtx_SET(rtx_reg_SP, rtx_SP_plus_N);
    rtx_call = gen_rtx_PARALLEL(VOIDmode, gen_rtvec(2, rtx_call, rtx_pop));

    rtx_call = emit_call_insn(rtx_call);
}

/* Emit a sequence of RTL instructions for pattern 'call_value_pop' */
void ivm64_expand_call_value_pop (rtx *operands)
{
    rtx rtx_retval, rtx_fnaddr, rtx_callarg1;
    rtx rtx_reg_SP, rtx_N, rtx_SP_plus_N, rtx_pop, rtx_call;

    rtx_retval   = operands[0];
    rtx_fnaddr   = operands[1];
    rtx_callarg1 = operands[2];
    rtx_N        = operands[4];

    rtx_call = gen_rtx_CALL(VOIDmode, rtx_fnaddr, rtx_callarg1);
    rtx_reg_SP = gen_rtx_REG(DImode, STACK_POINTER_REGNUM);
    rtx_SP_plus_N = gen_rtx_PLUS(Pmode, rtx_reg_SP, rtx_N);
    rtx_pop = gen_rtx_SET(rtx_reg_SP, rtx_SP_plus_N);
    rtx_call = gen_rtx_SET(rtx_retval, rtx_call);
    rtx_call = gen_rtx_PARALLEL (VOIDmode, gen_rtvec(2, rtx_call, rtx_pop));

    rtx_call = emit_call_insn(rtx_call);
}


void ivm64_expand_builtin_longjump_call(rtx *operands)
{
    if (!pushable_operand(operands[0], DImode))
        operands[0] = force_reg (DImode, operands[0]);

    /* the second arg of __builtin_longjmp is always 1 */
    rtx push_longjmp_operand2 = gen_pushdi1(GEN_INT(1));
    rtx push_longjmp_operand1 = gen_pushdi1(operands[0]);

    rtx fnaddr = gen_rtx_MEM (DImode, gen_rtx_SYMBOL_REF (Pmode, "longjmp"));
    rtx arg_bytes =  GEN_INT(GET_MODE_SIZE(DImode)*2);

    rtx call_longjmp = gen_call_pop(fnaddr, arg_bytes, NULL, arg_bytes);

    emit_insn(push_longjmp_operand2);
    emit_insn(push_longjmp_operand1);
    emit_insn(call_longjmp);
}

void ivm64_expand_builtin_longjump_inline(rtx *operands)
{
    rtx label = gen_label_rtx();

    char saved_pc_label_name[256];
    ASM_GENERATE_INTERNAL_LABEL(saved_pc_label_name, "LIVM_lj_pc",
                                CODE_LABEL_NUMBER(label));
    rtx saved_pc = gen_rtx_MEM(DImode, gen_rtx_SYMBOL_REF(DImode,
                                    xstrdup(saved_pc_label_name)));
    operands[0] = force_reg (DImode, operands[0]);

    rtx jmp_buff8 = gen_rtx_PLUS(DImode, operands[0],
                                 GEN_INT(GET_MODE_SIZE(Pmode)));   //pc
    rtx jmp_buff16 = gen_rtx_PLUS(DImode, operands[0],
                                 GEN_INT(2*GET_MODE_SIZE(Pmode))); //sp

    rtx old_pc = gen_rtx_MEM(DImode, jmp_buff8);
    ivm64_expand_push(old_pc, DImode);
    ivm64_expand_pop(saved_pc, DImode);

    rtx old_sp = gen_rtx_MEM(DImode, jmp_buff16);
    ivm64_expand_push(old_sp, DImode);
    /* set_sp */
    emit_insn(gen_rtx_UNSPEC_VOLATILE(DImode, gen_rtvec(1, CONST0_RTX(DImode)),
                                      UNSPEC_POP_SP));
    /* The second arg of __builtin_longjmp is always 1 */
    emit_insn(gen_rtx_SET(gen_rtx_REG(DImode, AR_REGNUM), GEN_INT(1)));
    emit_insn(gen_blockage());

    ivm64_expand_push(saved_pc, DImode);
    emit_insn(gen_rtx_SET(pc_rtx,
                          gen_rtx_UNSPEC_VOLATILE(DImode,
                            gen_rtvec(1, gen_rtx_REG(DImode, TR_REGNUM)),
                                      UNSPEC_POP_TR))
              ); // jump

    /* Remove "*" prefixed to the internal label */
    char *saved_pc_label_decl = xstrdup(saved_pc_label_name+1);
    /* Add ":" to the label declaration */
    saved_pc_label_decl = strncat(saved_pc_label_decl, ":", 255);
    emit_insn(gen_print_asm(gen_rtx_CONST_STRING(VOIDmode,
                                               xstrdup(saved_pc_label_decl))));
    emit_insn(gen_print_asm(gen_rtx_CONST_STRING(VOIDmode,"\tdata8[0]")));

    rtx fnaddr = gen_blockage();
    rtx call_rtx = gen_rtx_CALL(DImode, fnaddr, GEN_INT(0));
    emit_call_insn(call_rtx);
    emit_barrier();
}

void ivm64_expand_call_alloca(rtx *operands)
{
    if (!pushable_operand(operands[1], DImode))
        operands[1] = force_reg (DImode, operands[1]);

    rtx push_alloca_operand = gen_pushdi1(operands[1]);

    rtx fnaddr = gen_rtx_MEM (DImode, gen_rtx_SYMBOL_REF (Pmode, "alloca"));
    rtx ret_reg = gen_rtx_REG (DImode, AR_REGNUM);
    rtx arg_bytes =  GEN_INT(GET_MODE_SIZE(DImode)); // alloca() only one argument

    rtx call_alloca = gen_call_value_pop(ret_reg, fnaddr, arg_bytes, NULL, arg_bytes);

    emit_insn(push_alloca_operand);
    emit_insn(call_alloca);

    rtx new_operands[2];
    new_operands[0] = operands[0];
    new_operands[1] = ret_reg;
    ivm64_expand_move(new_operands, DImode);
}

void ivm64_expand_save_stack_block(rtx *operands ATTRIBUTE_UNUSED)
{
    rtx push_stack_save_operand = gen_pushdi1(GEN_INT(0));
    rtx fnaddr = gen_rtx_MEM(DImode,
                        gen_rtx_SYMBOL_REF(Pmode, "_builtin_stack_save_ivm64"));
    rtx ret_reg = gen_rtx_REG(Pmode, AR_REGNUM);
    rtx arg_bytes = GEN_INT(GET_MODE_SIZE(DImode));
    rtx call_stack_save = gen_call_value_pop(ret_reg, fnaddr, arg_bytes, NULL, arg_bytes);

    emit_insn(push_stack_save_operand);
    emit_insn(call_stack_save);
}

void ivm64_expand_restore_stack_block(rtx *operands ATTRIBUTE_UNUSED)
{
    rtx push_stack_restore_operand = gen_pushdi1(GEN_INT(0));
    rtx fnaddr = gen_rtx_MEM(DImode,
                      gen_rtx_SYMBOL_REF(Pmode, "_builtin_stack_restore_ivm64"));
    rtx arg_bytes = GEN_INT(GET_MODE_SIZE(DImode));
    rtx call_stack_restore = gen_call_pop(fnaddr, arg_bytes, NULL, arg_bytes);

    emit_insn(push_stack_restore_operand);
    emit_insn(call_stack_restore);
}


/* OUTPUT FUNCTIONS */

/* Return a string escaping special chars in order to show human readable
   strings. To be used in a comment when printing ascii data.
   The escaped string is passed by reference. Do not forget free this pointer
   after its use.  */
static long escape_str(const char *s, char **s_escaped_p, unsigned int maxlen){
    char buff[256];
    unsigned char si;
    unsigned int i, l=0;

    *s_escaped_p = (char*) xmalloc((strnlen(s,maxlen)+1)*4+1);
    char *s_escaped = *s_escaped_p;

    sprintf(s_escaped, "%s", "");
    for (i=0; i<strlen(s) && i<maxlen ; i++){
        si = (unsigned char)s[i];
        switch (si)
        {
            case '\0':
                strcat(s_escaped, "\\0"); break;
            case '\n':
                strcat(s_escaped, "\\n"); break;
            case '\r':
                strcat(s_escaped, "\\r"); break;
            case '\t':
                strcat(s_escaped, "\\t"); break;
            case '\\':
                strcat(s_escaped, "\\\\"); break;
            case '"':
                strcat(s_escaped, "\\\""); break;
            default:
                if (si>31 && si<127){
                    sprintf(buff, "%c", si);
                    l--;
                }
                else {
                    sprintf(buff, "\\x%02x", (unsigned char)si);
                    l += 2;
                }
                strcat(s_escaped, buff);
                break;
        }
        l += 2;
    }
    s_escaped[l+1]=0;
    return l;
}


/* Print an ascii string; used in the declaration of macro ASM_OUTPUT_ASCII.
   A string like "hello" will be printed as:
        data1 [ 104 101 108 108 111 0 ]
*/
#define IVM64_DATA_SMALL_ASCII_LEN 257
void ivm64_output_ascii(FILE *file, const char *ptr, int len)
{
    int i;

    /* For small strings, print the string in a comment for information */
    if (len < IVM64_DATA_SMALL_ASCII_LEN){
        char *ptr_e;
        escape_str(ptr, &ptr_e, len+1);
        fprintf (file, "#\t .string \"%s\"\n", ptr_e);
        free(ptr_e);
    }

    /* print the data directive */
    fprintf (file, "\tdata1 [");
    for (i=0 ; i < len ; i++){
        fprintf (file, " %d", (unsigned char)ptr[i]);
    }
    fprintf (file, " ]\n");
}

/* Print an integer rtx (number, symbol_ref, etc.) recursively */
static void ivm64_output_constant(FILE* file, rtx x)
{
    if (GET_CODE (x) == CONST_INT) {
        fprintf(file, "%ld", INTVAL(x));
    }
    else if ((GET_CODE (x) == SYMBOL_REF) || (GET_CODE (x) == LABEL_REF)){
        output_addr_const(file, x);
    }
    else if (GET_CODE (x) == CONST) {
        /* Patterns like (const (plus (...)) */
        ivm64_output_constant(file, XEXP(x,0));
    }
    else if ((GET_CODE (x) == PLUS) || (GET_CODE (x) == MINUS)) {
        /* Patterns like (plus (...) (...)) or (minus (...) (...)) */
        rtx op0 = XEXP (x, 0);
        rtx op1 = XEXP (x, 1);
        if (CONST_INT_P(op0) && CONST_INT_P(op1)){
            /* Pattern (plus (const_int) (const_int)) */
            rtx res;
            if (GET_CODE (x) == PLUS)
                res = GEN_INT(INTVAL(op0) + INTVAL(op1));
            else
                res = GEN_INT(INTVAL(op0) - INTVAL(op1));
            output_addr_const(file, res);
        } else {
            fprintf(file, "(+ ");
            ivm64_output_constant(file, op0);
            fprintf(file, " ");
            if (GET_CODE (x) == MINUS)
                fprintf(file, "-");
            ivm64_output_constant(file, op1);
            fprintf(file, ")");
        }
    }
    else {
        /* Fall back call for printing x */
        output_addr_const(file, x);
    }
}

/* The output template is a string which specifies how to output the assembler
   code for an instruction pattern. Most of the template is a fixed string
   which is output literally. The character ‘%’ is used to specify where to
   substitute an operand; it can also be used to identify places where different
   variants of the assembler require different syntax. In the simplest case, a
   ‘%’ followed by a digitn says to output operand n at that point in the string.
   ‘%’ followed by a letter and a digit says to output an operand in an alternate
   fashion.

   Four letters have standard, built-in meanings described below. The
   machine description macro PRINT_OPERAND can define additional letters with
   non standard meanings.

   %adigit’ can be used to substitute an operand as if it were a memory
   reference, with the actual operand treated as the address (not supported by the
   ivm6 target)

   ‘%cdigit’ can be used to substitute an operand that is a constant
   value without thesyntax that normally indicates an immediate operand.

   ‘%ndigit’ is like ‘%cdigit’ except that the value of the constant is negated
   before printing.

   ‘%ldigit’ is used to substitute a label_ref into a jump instruction.

   ‘%=’ outputs a number which is unique to each instruction in the entire
   compilation. This is useful for making local labels to be referred to more
   than once in a single template that generates multiple assembler instructions.

   ‘%’ followed by a punctuation character specifies a substitution that
   does not use an operand. Only one case is standard: ‘%%’ outputs a
   ‘%’ into the assembler code. Other non standard cases can be defined
   in the PRINT_OPERAND macro. You must also define which punctuation
   characters are valid with the PRINT_OPERAND_PUNCT_VALID_P macro.

   The template may generate multiple assembler instructions. Write the text
   for the instructions, with ‘\;’ between them. When the RTL contains two
   operands which are required by constraint to match each other, the output
   template must refer only to the lower-numbered operand. Matching operands are
   not always identical, and the rest of the compiler arranges to put the proper
   RTL expression for printing into the lower-numbered operand.
*/
/* Special letters for ivm64:
    h - print a CONST_INT in hexadecimal: 0x.....
*/
void print_operand(FILE *file, rtx x, int letter)
{
    /* There are two adjustments to take into account to locate the position
       of the top of the stack when printing an operand:
            - nesting of ivm64_output_push() calls (controled by variables
              emitted_push_cfun and emitted_pop_cfun),
            - explicit offset introduced via the ivm64_stack_extra_offset
              variable,
    */
    long delta = emitted_push_cfun - emitted_pop_cfun; /*stack units*/
    delta += ivm64_stack_extra_offset/UNITS_PER_WORD;

    /* Actual offset from SP to the first stack mapped GPR (AR) */
    long ivm64_gpr_offset_l = ivm64_gpr_offset + delta * UNITS_PER_WORD; /*bytes*/
    long ivm64_gpr_offset_l8 = ivm64_gpr_offset_l/UNITS_PER_WORD; /*stack units*/

    if (x == NULL){
        fputc(letter, file);
    }
    else if (REG_P (x))
    {
        /* ivm64: AR MUST be aligned to UNITS_PER_WORD */
        gcc_assert(ivm64_gpr_offset_l8 * UNITS_PER_WORD == ivm64_gpr_offset_l);
        if (REGNO(x) >=  (unsigned int)(FIRST_GP_REGNUM + used_gp_regs())) {
             ivm64_fatal("Register number out of range");
        }
        if (REGNO(x) == AR_REGNUM){
            // AR
            fprintf (file, "&%ld", ivm64_gpr_offset_l8);
        }
        else if (IS_GP_REGNUM(REGNO(x))) {
            // X1, X2, ....
            long regnorelx = REGNO(x) - FIRST_GP_REGNUM;
            fprintf (file, "&%ld", ivm64_gpr_offset_l8 + regnorelx);
        }
        else if (REGNO (x) == TR_REGNUM){
            // TR
            fprintf (file, "&0");
        }
        else {
            /* This branch should not be reacheable */
            /* SP is pushed explicitly as "push! &0" or popped as "set_sp"
               in ivm64_output_push() */
            fprintf (file, "%s", reg_names[REGNO (x)]);
            gcc_unreachable();
        }
    }
    else if (GET_CODE (x) == MEM)
    {
        int offset = delta; /*Stack units*/
        rtx addr = XEXP(x, 0);
        if (CONSTANT_ADDRESS_P(addr)){
            ivm64_output_constant(file, addr);
        }
        else if (GET_CODE(addr) == PLUS && REG_P(XEXP (addr, 0))
                && GET_CODE(XEXP(addr, 1)) == CONST_INT
                && REGNO(XEXP(addr, 0)) == STACK_POINTER_REGNUM) {
            long val = INTVAL (XEXP (addr, 1));
            if ( val % UNITS_PER_WORD){
                fprintf(file, "(+ &0 %ld)", INTVAL (XEXP (addr, 1))+ UNITS_PER_WORD * offset);
            } else {
                fprintf(file, "&%ld", INTVAL (XEXP (addr, 1))/UNITS_PER_WORD + offset);
            }
        }
        else if (REG_P(addr) &&
                REGNO(addr) == STACK_POINTER_REGNUM) {
            fprintf(file, "&%d", offset);
        }
        else if (REG_P(addr) || MEM_P(addr)) {
            /* recursive printing for indirect memory patterns in inlined asm */
            fprintf(file, "(load8 ");
            print_operand(file, addr, 0); // assembly syntax sugar
            fprintf(file, ")");
        }
        else {
            fprintf(file,"<invalid operand>");
            ivm64_fatal("Invalid operand");
        }
    }
    else if (GET_CODE (x) == CONST_INT && (letter == 'h')) {
        fprintf (file, "0x%0lx", (HOST_WIDE_INT)INTVAL(x));
    }
    else if (GET_CODE (x) == CONST_DOUBLE){
        long buff[2] = {0, 0}; /* 2 words needed for 64-bit reals (double) */
        real_to_target(buff, CONST_DOUBLE_REAL_VALUE (x), GET_MODE(x));
        fprintf (file, "0x%08x%08x", (uint32_t)buff[1], (uint32_t)buff[0]);
    }
    else {
        ivm64_output_constant (file, x);
    }
}


/* Main output function to push one operand */
void ivm64_output_push(rtx operand, enum machine_mode mode)
{
    rtx addr = XEXP(operand, 0);
    /* Allowed constraints: "i", "S", "t", "m" */
    if (satisfies_constraint_i(operand)){
        /* i */
        /* push immediate */
        output_asm_insn ("push! %0", &operand);
    } else if (satisfies_constraint_S(operand)
               || (REG_P(operand) && IS_GP_REGNUM(REGNO(operand))) ) {
        /* Q or t */
        /* push SP, LABEL or mem(SP+offset) */
        if (sp_register_operand(operand, mode)){
            /* push SP */
            output_asm_insn ("push! &0", NULL);
        } else if ((GET_CODE(operand) == MEM) && CONSTANT_ADDRESS_P(addr)) {
            /* push [label] */
            output_asm_insn(((mode==(DImode))||(mode==(DFmode))? "load8! %0" :
                             (mode==(SImode))||(mode==(SFmode))? "load4! %0" :
                             (mode==(HImode))? "load2! %0" : "load1! %0"),
                            &operand);
        } else {
            /* push sp[n] */
            output_asm_insn(((mode==(DImode))||(mode==(DFmode))? "load8! %0" :
                             (mode==(SImode))||(mode==(SFmode))? "load4! %0" :
                             (mode==(HImode))? "load2! %0" : "load1! %0"),
                            &operand);
        }
    } else if (satisfies_constraint_m(operand)) {
        /* m */
        ivm64_output_indirect_push(operand, mode);
    } else {
        fatal_insn ("ivm64: ivm64_output_push error", operand);
    }
}

/* Main output function to pop one operand */
void ivm64_output_pop(rtx operand, enum machine_mode mode)
{
    rtx addr = XEXP(operand, 0);
    /* Allowed constraints: "=S", "=t", "=m" */
    if (satisfies_constraint_S(operand)
               || (REG_P(operand) && IS_GP_REGNUM(REGNO(operand))) ) {
        /*Q or t*/
        if (REG_P(operand) && IS_GP_REGNUM(REGNO(operand))){
            /* pop GPR*/
            output_asm_insn ("store8! %0", &operand);
        } else if (sp_register_operand(operand, mode)){
            /* pop SP */
            output_asm_insn ("set_sp", NULL);
        } else if ((GET_CODE(operand) == MEM) && CONSTANT_ADDRESS_P(addr)) {
            /* pop [label] */
            output_asm_insn(((mode==(DImode))||(mode==(DFmode))? "store8! %0" :
                             (mode==(SImode))||(mode==(SFmode))? "store4! %0" :
                             (mode==(HImode))? "store2! %0" : "store1! %0"),
                             &operand);
        } else {
            /* pop sp[n] */
            output_asm_insn(((mode==(DImode))||(mode==(DFmode))? "store8! %0" :
                             (mode==(SImode))||(mode==(SFmode))? "store4! %0" :
                             (mode==(HImode))? "store2! %0" : "store1! %0"), &operand);
        }
    } else if (satisfies_constraint_m(operand)) {
        /* m */
        ivm64_output_indirect_pop(operand, mode);
    } else {
        fatal_insn ("ivm64: ivm64_output_pop error", operand);
    }
}

static void output_indirect_push_or_pop(rtx operand, enum machine_mode mode, int ispush)
{
    rtx offset = NULL;
    rtx address = XEXP(operand, 0);
    if (GET_CODE(address) == PLUS) {
        offset = XEXP(address, 1);
        address = XEXP(address, 0);
    }

    output_asm_insn ("load8! %0", &address);

    if (NULL != offset){
        output_asm_insn ("add! %0", &offset);
    }

    if (ispush) {
        output_asm_insn((mode==(DImode))||(mode==(DFmode))? "load8" :
                        (mode==(SImode))||(mode==(SFmode))? "load4" :
                        (mode==(HImode))? "load2" : "load1" , NULL);
    } else {
        output_asm_insn((mode==(DImode))||(mode==(DFmode))? "store8" :
                        (mode==(SImode))||(mode==(SFmode))? "store4" :
                        (mode==(HImode))? "store2" : "store1" , NULL);
    }
}

static void ivm64_output_indirect_push(rtx operand, enum machine_mode mode) {
    output_indirect_push_or_pop(operand, mode, 1);
}

static void ivm64_output_indirect_pop(rtx operand, enum machine_mode mode) {
    output_indirect_push_or_pop(operand, mode, 0);
}

static int ivm64_move_allows_fast_pop(rtx *operands)
{
    if (global_options.x_optimize == 0) return 0;

    return REG_P(operands[0]) && (REGNO(operands[0]) == AR_REGNUM)
           && (! reg_mentioned_p(operands[0], operands[1]))
           && (ivm64_stack_extra_offset == 0) && (ivm64_gpr_offset == 0)
           && (emitted_push_cfun == emitted_pop_cfun)
           && (! STACK_RELATIVE_P(operands[1]));
}

/* Output function to print a move instruction */
void ivm64_output_move(rtx *operands, enum machine_mode mode)
{
    if (ivm64_prolog_fast_pop) {
        // Fast pop after prologue
        if (!(REG_P(operands[0]) && REGNO(operands[0]) == AR_REGNUM)){
            gcc_unreachable();
        }
        ivm64_prolog_fast_pop = 0;
        ivm64_stack_extra_offset += - UNITS_PER_WORD;
        ivm64_output_push(operands[1], mode);
        ivm64_stack_extra_offset += + UNITS_PER_WORD;
    } else {
        int fast_pop = ivm64_move_allows_fast_pop(operands);

        if (!fast_pop) { // standard push/pop
            ivm64_output_push(operands[1], mode);
            emitted_push_cfun++;
            ivm64_output_pop(operands[0], mode);
            emitted_pop_cfun++;
        } else { // fast pop
            ivm64_output_setsp(asm_out_file, 1);
            ivm64_stack_extra_offset += - UNITS_PER_WORD;
            ivm64_output_push(operands[1], mode);
            emitted_push_cfun++;
            ivm64_stack_extra_offset += + UNITS_PER_WORD;
            emitted_pop_cfun++;
        }
    }
}

/* Output function to print a zero extend instruction */
void ivm64_output_zero_extend(int nbytes){
    if (nbytes < 8){
       int nbits = 8 * nbytes;
       uint64_t x0 = ((1UL << nbits) - 1);
       fprintf(asm_out_file, "\tand! %ld\n", x0);
    }
}

/* Output function to print a sign extend instruction */
void ivm64_output_sign_extend(int nbytes){
    if (nbytes < 8){
       int nbits = 8 * nbytes;
       uint64_t x0 = ((1UL << nbits) - 1);
       uint64_t x1 = (1UL << (nbits - 1));
       uint64_t x2 = (-1UL << nbits) + x1;
       fprintf(asm_out_file, "\tand! %ld\n", x0);
       fprintf(asm_out_file, "\txor! %ld\n", x1);
       fprintf(asm_out_file, "\tadd! %ld\n", x2);
    }
}

/* Output function to print a call instruction */
void ivm64_output_call(rtx address)
{
    if (satisfies_constraint_S(address)){
        /* direct call */
        output_asm_insn ("call! %0", &address);
    } else {
        /* indirect call */
        address = XEXP (address, 0);
        if ( (GET_CODE (address) == MEM
                && PC_OR_STACK_RELATIVE_P (XEXP (address, 0)))
             ||
             (GET_CODE(address) == REG)
           ) {
            output_asm_insn ("load8! %0", &address);
            output_asm_insn ("call", NULL);
        }
        else
            fatal_insn("ivm64: indirect call address error:", address);
    }
}

/*  Output instructions for moving the return value after a function call returns
    a value (patterns 'call_value' and 'call_value_pop').
    Immediately after returning from a call, the stack must be like this:

    +------------+    ^
    | arg N-1    |    | high
    +------------+
    |  ...       |
    +------------+
    | arg 1      |
    +------------+
    | arg 0      |
    +------------+
    | ret. val.  |  <- SP
    +------------+

    This function moves the return value (ret. val.) to the caller's AR register.
    For 2-word return values (e.g. complex double), the least significand word is
    moved to AR, and the most significand one to X1.

    If the argument restore_slots=1, the stack slots left by the return value
    are restored via 'push 0', in order to keep the same number of words as pops_args.
    This is thought for pattern 'call_value', where it is gcc who emits
    'sp <- sp +pop_args' after the call.

    Nevertheless if 'caller pops args' is done in the 'call_value_pop'/'call_pop'
    patterns, you can save these 'push 0' substracting the number of 'push 0' from
    the pop_args amount, that is, 'sp <- sp + (pops_args - restored_slots)', because
    in these cases it is the pattern who print the update of SP.

    Function ivm64_output_return_value() returns the number of stack slots needed
    to compensate the movement of the return values (to AR/X1)
*/
long ivm64_output_return_value(rtx *operands, int restore_slots)
{
    int ret_regs; /* Number of registers used for the return value
                     (SC,DI,SI,... -> AR; DC, CDI, TI -> (X1,AR))*/
    ret_regs = (DCmode == GET_MODE(operands[0])
                || CDImode == GET_MODE(operands[0])
                || TImode == GET_MODE(operands[0]))?2:1;

    /* Argument space in bytes*/
    long args_nbytes = INTVAL(operands[2]);

    long restored_slots = 0;

    rtx dst_reg;
    int dst_regno = AR_REGNUM;
    if ((args_nbytes > UNITS_PER_WORD) || (args_nbytes==UNITS_PER_WORD && ret_regs==1 ) ){
        /* When there are arguments two cases need to be considered:
             - either more than one argument
             - or only one argument and the return value fits in one word (DI,SI,..)
           (one argument and DCmode is a special case) */
        for (long i=ret_regs; i>0; i--){
            if (args_nbytes % UNITS_PER_WORD){
                rtx args_nbytes_rtx = GEN_INT(args_nbytes) ; // bytes
                output_asm_insn("store8! (+ &0 %0)", &args_nbytes_rtx);
            } else {

                dst_reg = gen_rtx_REG(DImode, dst_regno);
                output_asm_insn("store8! %0", &dst_reg);
                ivm64_stack_extra_offset -=  UNITS_PER_WORD; //bytes
            }
            dst_regno++;
        }
        ivm64_stack_extra_offset +=  ret_regs * UNITS_PER_WORD ; //bytes

        /* Compensate the store of the return value
           Next RTL introduced by gcc will be SP<-SP-args_nbytes */
        for (long i=ret_regs; i>0; i--){
            if (restore_slots)
                output_asm_insn("push! 0",NULL);
            restored_slots++;
        }
    } else if (args_nbytes==UNITS_PER_WORD && ret_regs==2 ) {
        /* When the return value is double complex (ret_regs=2 words)
           but there is only one argument */

        dst_reg = gen_rtx_REG(DImode, AR_REGNUM);
        output_asm_insn("load8! %0", &dst_reg);
        ivm64_stack_extra_offset +=  UNITS_PER_WORD; //bytes
        dst_reg = gen_rtx_REG(DImode, X1_REGNUM);
        output_asm_insn("store8! %0", &dst_reg);
        ivm64_stack_extra_offset -=  UNITS_PER_WORD; //bytes
        dst_reg = gen_rtx_REG(DImode, AR_REGNUM);
        output_asm_insn("store8! %0", &dst_reg);

        if (restore_slots)
            output_asm_insn("push! 0",NULL);
        restored_slots++;
    } else {
        /* When there is no arguments -> args_nbytes == 0
           do nothing, the return value has been placed on the stack
           and there are no arguments to release */
    }

    return restored_slots;
}

/* Return an RTX expression extending a constant with zeros */
static rtx ivm64_const_int_zero_extend(rtx r, int modesize)
{
    /* modesize is in bytes */
    if (CONST_INT_P(r)){
        unsigned long val = INTVAL(r);
        if (modesize == 1) r = GEN_INT(val & 0x0ff);
        if (modesize == 2) r = GEN_INT(val & 0x0ffff);
        if (modesize == 4) r = GEN_INT(val & 0x0ffffffff);
    }
    return r;
}

/* Reverse a comparison operator */
static enum rtx_code ivm64_reverse_compare_code(enum rtx_code code)
{
    switch(code) {
        case GT:
            code = LT; break;
        case LT:
            code = GT; break;
        case GE:
            code = LE; break;
        case LE:
            code = GE; break;
        case GTU:
            code = LTU; break;
        case LTU:
            code = GTU; break;
        case GEU:
            code = LEU; break;
        case LEU:
            code = GEU; break;
        default:
            break;
    }
    return code;
}

/* Print a compare and branch action for two operands in the best possible way
   to reduce the number of native ivm64 instructions */
void ivm64_output_cbranch(rtx *operands, machine_mode mode)
{
    enum rtx_code code;
    code = GET_CODE(operands[0]);

    int swapping=0; // swap operands
    int negated=0;  // negate condition
    int inskind=0;  // 0: signed lt_s; 1:unsigned lt_u; 2:eq/ne

    swapping = (code == GT) || (code == LE) || (code == GTU) || (code == LEU);
    negated =  (code == GE) || (code == LE) || (code == GEU) || (code == LEU) || (code == NE);
    inskind =  ((code == GTU) || (code == LTU) || (code == GEU) || (code == LEU))?1
                :((code == EQ) || (code == NE))?2
                :0;

    int modesize = GET_MODE_SIZE(mode); /*bytes*/

    /* Adapt integer constants to the size of the mode (zero_extend) */
    operands[1] = ivm64_const_int_zero_extend(operands[1], modesize);
    operands[2] = ivm64_const_int_zero_extend(operands[2], modesize);

    rtx operand0, operand1;
    if (swapping) {
        operand0 = operands[2];
        operand1 = operands[1];
    } else {
        operand0 = operands[1];
        operand1 = operands[2];
    }

    if (((NE == code) || (EQ == code))
        && (rtx_equal_p (operands[1], const0_rtx)
            || rtx_equal_p (operands[2], const0_rtx))
    ) {
        /* Stack adjust not necessary as this push is swallowed by the
           jump; no comparison emitted when comparing with 0 */
        if (rtx_equal_p (operands[1], const0_rtx))
            ivm64_output_push(operands[2], mode);
        else
            ivm64_output_push(operands[1], mode);
        negated = (EQ == code);
        if (negated) {
            output_asm_insn("jump_zero! %3", operands);
        } else {
            output_asm_insn("jump_not_zero! %3", operands);
        }
        return;
    }  else if (EQ == code) {
        if (REG_P(operand1) && (REGNO(operand1) == AR_REGNUM)){
            rtx r;
            r = operand0;
            operand0 = operand1;
            operand1 = r;
        }

        ivm64_output_push(operand0, GET_MODE(operand0));
        ivm64_stack_extra_offset += UNITS_PER_WORD;

        ivm64_output_push(operand1, GET_MODE(operand1));

        output_asm_insn("xor", NULL);
        ivm64_stack_extra_offset -= UNITS_PER_WORD;

        output_asm_insn("jump_zero! %3", operands);
        return;
    } else if ( ((LT == code) || (GE == code))
        && rtx_equal_p(operands[2], const0_rtx))
    {
        rtx shift = GEN_INT(1UL << (8*GET_MODE_SIZE(mode) - 1));

        int do_mul1 = (LE == code) || (GT == code);
        int do_ltu = (LT == code) || (GT == code);

        ivm64_output_push(operands[1], mode);
        if (do_mul1) {
            output_asm_insn("mult! -1", NULL);
        }
        output_asm_insn("and! %h0", &shift);
        if (do_ltu) {
            output_asm_insn("lt_u! 1", NULL);
        }
        output_asm_insn("jump_zero! %3", operands);
        return;
    } else if ( ((GT == code) || (GE == code)) && CONST_INT_P(operands[2])) {
        rtx const2_N1 = GEN_INT((1UL) << (8*modesize-1));

        ivm64_output_push(operands[1], mode);

        ivm64_stack_extra_offset += UNITS_PER_WORD;
        output_asm_insn("xor! %h0", &const2_N1);

        unsigned long v = INTVAL(const2_N1) + INTVAL(operands[2]);
        if (GT == code) v +=  1;
        if (modesize < 8) v = v &  ((1UL << (modesize*8)) -1);

        rtx vrtx = GEN_INT(v);

        output_asm_insn("lt_u! %0", &vrtx);

        output_asm_insn("jump_zero! %3", operands);
        ivm64_stack_extra_offset -= UNITS_PER_WORD;

        return;

    } else {
        rtx const2_N1 = GEN_INT((1UL) << (8*modesize-1));

        if (inskind==0) { // signed lt_s -> lt_u
            ivm64_output_push(operand0, GET_MODE(operand0));
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            output_asm_insn("xor! %h0", &const2_N1);

            ivm64_output_push(operand1, GET_MODE(operand1));
            output_asm_insn("xor! %h0", &const2_N1);
        } else if (inskind==1) { // native lt_u
            ivm64_output_push(operand0, GET_MODE(operand0));
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            ivm64_output_push(operand1, GET_MODE(operand1));
        } else {    // eq/ne
            if (REG_P(operand1) && (REGNO(operand1) == AR_REGNUM)){
                rtx r;
                r = operand0;
                operand0 = operand1;
                operand1 = r;
            }

            ivm64_output_push(operand0, GET_MODE(operand0));
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            ivm64_output_push(operand1, GET_MODE(operand1));
            output_asm_insn("xor", NULL);

            ivm64_output_push(const1_rtx, DImode);
        }
        output_asm_insn("lt_u", NULL);
        ivm64_stack_extra_offset -= UNITS_PER_WORD;
    }

    if (! negated)
        output_asm_insn("not", NULL);
    output_asm_insn("jump_zero! %3", operands);

}



static int ivm64_rtx_is_and_const(rtx x, unsigned long *val) {
    int ret = 0;
    if (GET_CODE(x) == SET){
       rtx op0 = XEXP(x, 0);
       if (REG_P(op0) && REGNO(op0) == TR_REGNUM) {
            rtx op1 = XEXP(x, 1);
            if (GET_CODE(op1) == AND) {
                rtx op1_0 = XEXP(op1, 0);
                rtx op1_1 = XEXP(op1, 1);
                if (REG_P(op1_0) && REGNO(op1_0) == TR_REGNUM
                    && CONST_INT_P(op1_1))
                {
                    ret = 1;
                    *val = INTVAL(op1_1);
                }
            }
       }
    }
    return ret ;
}

static int ivm64_rtx_is_unspec_push(rtx x, machine_mode *mode) {
    int ret = 0;
    if (GET_CODE(x) == SET){
       rtx op0 = XEXP(x, 0);
       if (REG_P(op0) && REGNO(op0) == TR_REGNUM) {
            rtx op1 = XEXP(x, 1);
            int op1code = GET_CODE(op1);
            if ( op1code == UNSPEC || op1code == UNSPEC_VOLATILE) {
                int unspec_code = XINT(op1,1);
                if (unspec_code == UNSPEC_PUSH_TR) {
                    ret = 1;
                    *mode = GET_MODE(op0);
                }
            }
       }
    }
    return ret;
}

/* Print a compare and branch action for one operand in the best possible way
   to reduce the number of native ivm64 instructions; the first operand
   is already on the stack. To be used in peepholes. */
void ivm64_output_cbranch_peep(rtx *operands, machine_mode mode, int reverse, rtx_insn *insn)
{
    /* Update emitted pop counter as the first comparand has been already pushed */
    emitted_pop_cfun++;

    enum rtx_code code;
    code = GET_CODE(operands[0]);

    if (reverse) {
        code = ivm64_reverse_compare_code(code);
    }

    int negated=0;  // negate condition
    int inskind=0;  // 0: signed lt_s; 1:unsigned lt_u; 2:eq/ne
    int swapping=0; // swap operands
    rtx operand1 = operands[2];

    swapping = (code == GT) || (code == LE) || (code == GTU) || (code == LEU);
    negated =  (code == GE) || (code == LE) || (code == GEU) || (code == LEU) || (code == NE);
    inskind =  ((code == GTU) || (code == LTU) || (code == GEU) || (code == LEU))?1
                :((code == EQ) || (code == NE))?2
                :0;

    int modesize = GET_MODE_SIZE(mode); /*bytes*/
    unsigned long mask0extend = -1L;
    if (modesize == 4) {mask0extend = 0x0ffffffff;}
    if (modesize == 2) {mask0extend = 0x0ffff;}
    if (modesize == 1) {mask0extend = 0x0ff;}

    rtx prevrtx = PATTERN(PREV_INSN(insn));
    unsigned long andval = 0;
    int previsand = ivm64_rtx_is_and_const(prevrtx, &andval);
    machine_mode pushmode;
    int pushmodesize = 0;
    int previspush = ivm64_rtx_is_unspec_push(prevrtx, &pushmode);
    if (previspush) pushmodesize = GET_MODE_SIZE(pushmode);


    /* Adapt integer constants to the size of the mode (zero_extend) */
    operand1 = ivm64_const_int_zero_extend(operand1, modesize);

    int op1zero = rtx_equal_p(operand1, const0_rtx);
    /* Let's zero extend the operand already in the stack
       (i.e., this one supressed by the peephole) */
    rtx const0extend = GEN_INT(mask0extend);
    if (modesize < 8
        && !( (previsand && (andval < mask0extend))
              || (previspush && (pushmodesize <= modesize))
            )
    ){
        output_asm_insn("and! %0", &const0extend);
    }

    if (((NE == code) || (EQ == code)) && (op1zero)) {
        /* Stack adjust not necessary as this push is swallowed by the
           jump; no comparison emitted when comparing with 0 */
        negated = (EQ == code);
        if (negated) {
            output_asm_insn("jump_zero! %1", operands);
        } else {
            output_asm_insn("jump_not_zero! %1", operands);
        }
        return;
    } else if (EQ == code) {
        ivm64_stack_extra_offset += UNITS_PER_WORD;
        ivm64_output_push(operand1, GET_MODE(operand1));

        output_asm_insn("xor", NULL);
        ivm64_stack_extra_offset -= UNITS_PER_WORD;

        output_asm_insn("jump_zero! %1", operands);
        return;
    }  else {
        rtx const2_N1 = GEN_INT((1UL) << (8*modesize-1));

        if (inskind==0) { // signed lt_s -> lt_u
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            output_asm_insn("xor! %h0", &const2_N1);

            ivm64_output_push(operand1, GET_MODE(operand1));
            output_asm_insn("xor! %h0", &const2_N1);
        } else if (inskind==1) { // native lt_u
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            ivm64_output_push(operand1, GET_MODE(operand1));
        } else {    // eq/ne
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            ivm64_output_push(operand1, GET_MODE(operand1));
            output_asm_insn("xor", NULL);
            ivm64_output_push(const1_rtx, DImode);
        }

        if (swapping)
            output_asm_insn("gt_u", NULL);
        else
            output_asm_insn("lt_u", NULL);
        ivm64_stack_extra_offset -= UNITS_PER_WORD;
    }

    if (! negated)
        output_asm_insn("not", NULL);
    output_asm_insn("jump_zero! %1", operands);

}


/* Return true if it is safe to apply some compare and branch
   peepholes where the first operand was already on the stack.
   This is safe only if swapping operands is not required.*/
int ivm64_peep_pop_cmp_p(rtx op, int reverse) {
    enum rtx_code code = GET_CODE(op);
    if (reverse)
        code = ivm64_reverse_compare_code(code);
    if ( (code == LE) || (code == LEU) || (code == GT) || (code == GTU) ) {
       return 0;
    }
    return 1;
}

/* Print instructions for pattern 'casesi'
    operand 0: index
    operand 1: lower bound
    operand 2: range = upper bound - lower bound
    operand 3: code_label for the table
    operand 4: out-of-range label */
void ivm64_output_casesi(rtx *operands)
{
    machine_mode mode = GET_MODE(operands[0]);
    int modesize = GET_MODE_SIZE(mode); /*bytes*/
    int modebits = modesize * 8; /*bits*/

    uint64_t x0 = (1UL << modebits) - 1;

    uint64_t range_val = INTVAL(operands[2]) + 1UL;
    rtx range = GEN_INT(range_val);
    uint64_t lowerbound_val = -1UL * INTVAL(operands[1]);
    rtx lowerbound = GEN_INT(lowerbound_val);
    if (modesize == 8 || range_val <= x0){
        operands[0] = ivm64_const_int_zero_extend(operands[0], modesize);
        ivm64_output_push(operands[0], mode);
        ivm64_stack_extra_offset += UNITS_PER_WORD;

        if (lowerbound_val != 0) {
            output_asm_insn("add! %0", &lowerbound);
            fprintf(asm_out_file, "\tand! %ld\n", x0);
        }

        /* (unsigned)(index - lower_bound) > range */
        output_asm_insn("lt_u! %0", &range);

        /* jump to the out-of-range label*/
        output_asm_insn("jump_zero! %4", operands);
        ivm64_stack_extra_offset -= UNITS_PER_WORD;
    }

    /* compute (index - lower bound)*sizeof(long)
       to index the dispatch table*/
    rtx word_size = GEN_INT(UNITS_PER_WORD);

    ivm64_output_push(operands[0], mode);
    ivm64_stack_extra_offset += UNITS_PER_WORD;

    if (lowerbound_val != 0) {
        output_asm_insn("add! %0", &lowerbound);
        fprintf(asm_out_file, "\tand! %ld\n", x0);
    }

    output_asm_insn("mult! %0", &word_size);
    output_asm_insn("add! %0", &operands[3]);
    /* load the selected dispatch table entry */
    output_asm_insn("load8", NULL);
    if (CASE_VECTOR_PC_RELATIVE){
        output_asm_insn("add! %0", &operands[3]);
    }
    output_asm_insn("jump", NULL);
    ivm64_stack_extra_offset -= UNITS_PER_WORD;
}

/* Print an instruction to move SP n stack slots, that is,
   instruction 'set_sp! &n' */
void ivm64_output_setsp(FILE *file, long n)
{
    if (n == 0){
        return; /* Leave SP as it is */
    }
    if (n == 1) {
        /* Use 'store8! &0' instead of 'set_sp &1' as it is faster*/
        fprintf(file, "\tstore8! &0");
    }else if (n == 2) {
        fprintf(file, "\tstore8! &0\n");
        fprintf(file, "\tstore8! &0");
    } else {
        fprintf(file, "\tset_sp! &%ld", n);
    }
    fprintf(file, "\n");
}


/** TARGET HOOKS **/

void ivm64_asm_globalize_label(FILE *f, const char *name)
{
    fputs (GLOBAL_ASM_OP, f);
    assemble_name (f, name);
    fputs ("\n", f);
}

void ivm64_asm_file_start(void)
{
    default_file_start();
    fputs(ASM_COMMENT_START " " VERSION_INFO "\n", asm_out_file);
    fputs("#", asm_out_file);
    output_file_directive(asm_out_file, main_input_filename);
}

void ivm64_init_libfuncs ()
{
}

static void ivm64_option_override()
{
    extern struct gcc_options global_options;

    #if (! IVM64_OMIT_FP_MESSAGE)
    if (!flag_omit_frame_pointer)
        warning(0, "-fomit-frame-pointer always active!");
    #endif
    flag_omit_frame_pointer = 1;
    /* pops cannot be deferred, to have control on the offset to get GPRs */
    #if (! IVM64_OMIT_FP_MESSAGE)
    if (flag_defer_pop)
        warning(0, "-fdefer-pop not allowed");
    #endif
    flag_defer_pop = 0;
    global_options.x_flag_tree_ter = 0;
    global_options.x_flag_dse = 0;
    global_options.x_flag_crossjumping = 0;
    global_options.x_flag_delete_null_pointer_checks = 0;
    global_options.x_flag_dce= 0;
    global_options.x_flag_isolate_erroneous_paths_dereference = 0;
    global_options.x_flag_tree_loop_vectorize = 0;
    global_options.x_flag_tree_slp_vectorize = 0;
    global_options.x_flag_unsafe_math_optimizations = 0;
    global_options.x_flag_finite_math_only= 0;
    flag_reorder_blocks_algorithm = REORDER_BLOCKS_ALGORITHM_STC;
    if (lang_GNU_CXX()){
        global_options.x_flag_tree_dse = 0;
    }
    #if 1
    {
        int peephole2_disabled = 0;
        unsigned int i;
        cl_deferred_option *opt;
        vec<cl_deferred_option> v;
        if (common_deferred_options)
            v = *((vec<cl_deferred_option> *) common_deferred_options);
        else
            v = vNULL;
        FOR_EACH_VEC_ELT (v, i, opt)
        {
            if (opt->opt_index == OPT_fdisable_ && !strcmp("rtl-peephole2", opt->arg)){
                peephole2_disabled = 1;
                break;
            }
        }
        if ((global_options.x_optimize > 0) && !peephole2_disabled) {
            ivm64_extra_peep2_pass_number[0] =
               g->get_passes()->get_pass_peephole2()->static_pass_number; /* pass id */
            ivm64_add_extra_peep2_pass("peephole2");
            ivm64_add_extra_peep2_pass("ce3");
            ivm64_add_extra_peep2_pass("rnreg");
        }
    }
    #endif
}

unsigned int ivm64_hard_regno_nregs(unsigned int regno ATTRIBUTE_UNUSED,
                                    machine_mode mode)
{
    return ((GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD);
}

bool ivm64_hard_regno_mode_ok(unsigned int regno ATTRIBUTE_UNUSED,
                              machine_mode mode)
{
    return (mode == QImode    || mode == SImode || mode == DImode
            || mode == HImode || mode == SFmode || mode == DFmode
            || mode == SCmode
           );
}

bool ivm64_modes_tieable_p(machine_mode mode1 ATTRIBUTE_UNUSED,
                           machine_mode mode2 ATTRIBUTE_UNUSED)
{
    return 1;
}

HOST_WIDE_INT ivm64_starting_frame_offset()
{
    /* If FRAME_GROWS_DOWNWARD, this is the offset to end of the first slot
       allocated */
    return -1*(UNITS_PER_WORD);
}

rtx ivm64_legitimize_address (rtx x, rtx oldx ATTRIBUTE_UNUSED,
                              machine_mode mode ATTRIBUTE_UNUSED)
{
    return x;
}

 void ivm64_function_arg_advance(cumulative_args_t ca ATTRIBUTE_UNUSED,
                         const function_arg_info &arg ATTRIBUTE_UNUSED)
{
    return; /* Do nothing*/
}

rtx ivm64_function_arg(cumulative_args_t ca ATTRIBUTE_UNUSED,
                      const function_arg_info &arg ATTRIBUTE_UNUSED)
{
    return 0; /* Pass arguments on the stack*/
}

rtx ivm64_struct_value_rtx(tree fndecl ATTRIBUTE_UNUSED,
                           int incoming ATTRIBUTE_UNUSED)
{
    return 0; /* When passing a structura as argument, return 0 if
                 the address is passed as an “invisible” first argument */
}

bool ivm64_legitimate_constant_p(machine_mode mode ATTRIBUTE_UNUSED, rtx x)
{
   return (GET_CODE (x) == LABEL_REF || GET_CODE (x) == SYMBOL_REF
           || GET_CODE(x) == CONST_INT || GET_CODE (x) == CONST
           || GET_CODE (x) == CONST_DOUBLE);
}

bool ivm64_truly_noop_truncation(poly_uint64 outprec ATTRIBUTE_UNUSED,
                                 poly_uint64 inprec ATTRIBUTE_UNUSED)
{
    return 1;
}

/* This funtion returns the initial difference between the specified pair of
   registers. Used to define INITIAL_ELIMINATION_OFFSET. */
HOST_WIDE_INT ivm64_initial_elimination_offset (int from, int to)
{
    /* Define this hook together with TARGET_STARTING_FRAME_OFFSET */

    /* Example of the stack layout after the prologue:

    foo(long x, long y) {
       long i,j,k;
    }

                     |  ...              |
                     +-------------------+
                     | caller's AR reg.  |
                     +-------------------+
                     | y                 |\
                     +-------------------+ | arguments
                     | x                 |/
                     +-------------------+
               FP -> | return address    |
                     +-------------------+
                    /| i                 |
 get_frame_size()  | +-------------------+ <-TARGET_STARTING_FRAME_OFFSET
 = frame size = 3  | | j                 |   = -1 * UNITS_PER_WORD
                   | +-------------------+
                    \| k                 |
                     +-------------------+
                     | callee's Xn-1 reg.|
                     +-------------------+
                     |    ...            |
                     +-------------------+
                     | callee's X2 reg.  |
                     +-------------------+  |
                     | callee's X1 reg.  |  |
                     +-------------------+ \|/
               SP -> | callee's AR reg.  |  ' stack grows
                     +-------------------+    downwards
                     |  ...              |

    SP = Stack Pointer, the only hard register existing in this target
    FP = Frame Pointer, a hard register that can never appear in the output
         because must be eliminated according to FP = SP + elimination_offset

    The stack frame starts TARGET_STARTING_FRAME_OFFSET below FP,
    consequently TARGET_STARTING_FRAME_OFFSET must return (-UNITS_PER_WORD)
    */

    long offset = 0;
    long fs = get_frame_size();
    fs += (used_gp_regs() - 1)*UNITS_PER_WORD;
    if (from == FRAME_POINTER_REGNUM && to == STACK_POINTER_REGNUM){
        offset = fs + 1*UNITS_PER_WORD - (ivm64_starting_frame_offset());
    }
    return offset;
}


/* This target hook returns the number of bytes of its own arguments that a
   function pops on returning, or 0 if the function pops no arguments and the
   caller must therefore pop them all after the function returns.  fundecl is a
   C variable whose value is a tree node that describes the function in
   question. Normally it is a node of type FUNCTION_DECL that describes the
   declaration of the function. From this you can obtain the DECL_ATTRIBUTES of
   the function.  funtype is a C variable whose value is a tree node that
   describes the function in question. Normally it is a node of type
   FUNCTION_TYPE that describes the data type of the function. From this it is
   possible to obtain the data types of the value and arguments (if known).

   FUNCTION_TYPE used to represent the type of non-member functions and of
   static member functions. The TREE_TYPE gives the return type of the
   function. The TYPE_ ARG_TYPES are a TREE_LIST of the argument types. The
   TREE_VALUE of each node in this list is the type of the corresponding
   argument; the TREE_PURPOSE is an expression for the default argument value,
   if any. If the last node in the list is void_list_node (a TREE_LIST node
   whose TREE_VALUE is the void_type_ node), then functions of this type do not
   take variable arguments. Otherwise, they do take a variable number of
   arguments.  Note that in C (but not in C++) a function declared like void
   f() is an unpro- totyped function taking a variable number of arguments; the
   TYPE_ARG_TYPES of such a function will be NULL. */
poly_int64 ivm64_return_pops_args(tree fundecl ATTRIBUTE_UNUSED,
                                tree funtype ATTRIBUTE_UNUSED, poly_int64 size)
{
    /* This makes all functions behave as variadic. Patterns 'call_pop'
       and 'call_value_pop' will be in charge of popping the arguments after
       the call */
    return size;
}

static int ivm64_next_insn_allows_fast_pop(rtx_insn *insn)
{
    if (global_options.x_optimize == 0) return 0;

    while (insn) {
        if (LABEL_P(insn) || active_insn_p(insn)) {
            break;
        }
        insn = NEXT_INSN(insn); // purge notes, debug
    }

    if (insn && GET_CODE(insn) == INSN) {
        rtx set = single_set(insn);
        if (set != NULL) {
            rtx dest = SET_DEST(set);
            rtx src = SET_SRC(set);
            if (REG_P(dest) && REGNO(dest) == AR_REGNUM
                && pushable_operand(src, GET_MODE(src))
                && ! reg_mentioned_p(dest, src)
                && (! STACK_RELATIVE_P(src))
            ) {
                return UNITS_PER_WORD;
            }
        }
    }
    return 0;
}



static void ivm64_asm_function_prologue(FILE *file)
{
    /* Reset offsets and counters*/
    ivm64_gpr_offset = 0;
    ivm64_stack_extra_offset = 0;
    emitted_push_cfun = 0;
    emitted_pop_cfun = 0;

    /* Frame size */
    long size = get_frame_size(); /*bytes*/
    /* Add the space needed for GPRs */
    size += used_gp_regs() * UNITS_PER_WORD;

    if (frame_pointer_needed){
        const char *msg = ASM_COMMENT_START "Frame pointer not eliminated";
        fputs(msg, file);
        ivm64_fatal(msg);
    }

    ivm64_prolog_fast_pop = 0;
    if (size <=5 *UNITS_PER_WORD)
        ivm64_prolog_fast_pop = ivm64_next_insn_allows_fast_pop(get_insns());
    int allocated_size = size - ivm64_prolog_fast_pop;

    /* Print instructions to allocate space for local vars. and used GPRs */
    if (allocated_size <= 4 * UNITS_PER_WORD){
        for (int k=0; k < allocated_size/UNITS_PER_WORD; k++){
             fprintf(file, "\tpush! 0\n");
        }
    } else {
        ivm64_output_setsp(file, -allocated_size/UNITS_PER_WORD);
    }

}


static void ivm64_asm_function_epilogue(FILE *file)
{
    /* In the current ABI, patterns "call_pop" and "call_value_pop" are in
       charge of printing the SP update, popping arguments after the call. */

    long size = get_frame_size(); /*bytes*/
    /* Add the space needed for used GPRs */
    size += used_gp_regs() * UNITS_PER_WORD;
    long size8 = size/UNITS_PER_WORD; /*stack units*/

    int void_type; /* Is the function returning void? */
    void_type = VOID_TYPE_P(TREE_TYPE(DECL_RESULT(current_function_decl)));

    if (TREE_THIS_VOLATILE(current_function_decl)) {
        // This function DOES NOT return
        output_asm_insn("exit", NULL);
    } else {
        if (! void_type){
            /* Number of registers needed for the return value: AR for 1-word
               modes (QI,HI,SI,DI,SF,DF,SC); (X1, AR) for 2-word modes (DC) */
            int ret_regs;
            enum machine_mode tree_mode = TYPE_MODE(TREE_TYPE(DECL_RESULT (current_function_decl)));
            ret_regs = (DCmode == tree_mode || CDImode == tree_mode || TImode == tree_mode )?2:1;

            size8 -= ret_regs; /* Return registers are eventually popped */

            /* Let gcc store the return value to the caller's AR (see hook
               TARGET_FUNCTION_VALUE). Nevertheless with option
               '-fno-flag-tree-coalesce-vars', the epilogue will be in charge of
               storing the return value into the caller's AR */
            if (RETURN_ON_ARGS) {
                ivm64_output_setsp(file, (long)(size8 + ret_regs)); /* release frame */
            } else {
                if (1 == ret_regs){
                    fprintf(file,  "\tstore8! &%ld\n", (long)(size8) + 2);
                } else {
                    fprintf(file,  "\tstore8! &%ld\n", (long)(size8) + 3);
                    fprintf(file,  "\tstore8! &%ld\n", (long)(size8) + 3);
                }
                ivm64_output_setsp(file, (long)size8); /* release (frame - ret_regs) */
            }
        } else {
            /* no return value */
            ivm64_output_setsp(file, (long)size8); /* release frame */
        }

        fprintf(file, "\treturn\n");
    }

    if (ivm64_gpr_offset != 0
        || ivm64_stack_extra_offset !=0
        || ( emitted_push_cfun != emitted_pop_cfun) )
    {
        char msg[1024];
        sprintf(msg,  "ivm64 error: in '%s' not balanced stack at epilogue\n", current_function_name());
        fprintf(file,  ASM_COMMENT_START " %s", msg);
        ivm64_fatal(msg);
    }

    fprintf(file, "\n");
    fprintf(file, ASM_COMMENT_START " FUNCTION ENDS: %s\n\n", current_function_name());

    fflush(NULL);
}


bool
ivm64_use_by_pieces_infrastructure_p(unsigned HOST_WIDE_INT size ATTRIBUTE_UNUSED,
              unsigned int alignment ATTRIBUTE_UNUSED,
              enum by_pieces_operation op ATTRIBUTE_UNUSED,
              bool speed_p ATTRIBUTE_UNUSED)
{
    return 0; /* Return false, do not copy mem blocks by pieces */
}

unsigned int ivm64_case_values_threshold (void)
{
    /* This returns the smallest number of different values
       for which it is best to use a jump-table instead of a tree of
       conditional branches. The default is four for machines with a casesi
       instruction and five otherwise. This is best for most machines. */
    return 5;
}

#if (IVM64_DISABLE_INLINE == 1)
static bool ivm64_can_inline_p(tree caller ATTRIBUTE_UNUSED,
                               tree callee ATTRIBUTE_UNUSED)
{
    /* To disable inlining completely */
    return 0;
}
#endif

static void ivm64_asm_out_ctor (rtx symbol ATTRIBUTE_UNUSED,
                                int priority ATTRIBUTE_UNUSED)
{
    fputs ("#\t.global __do_global_ctors\n", asm_out_file);
    if (GET_CODE(symbol) == SYMBOL_REF){
        fprintf(asm_out_file, "#:ivm64:#CTOR %s\n", XSTR(symbol, 0));
    }
}

static void ivm64_asm_out_dtor (rtx symbol ATTRIBUTE_UNUSED,
                                int priority ATTRIBUTE_UNUSED)
{
    fputs ("#\t.global __do_global_dtors\n", asm_out_file);
    if (GET_CODE(symbol) == SYMBOL_REF){
        fprintf(asm_out_file, "#:ivm64:#DTOR %s\n", XSTR(symbol, 0));
    }
}


#undef TARGET_REGISTER_MOVE_COST
#define TARGET_REGISTER_MOVE_COST ivm64_register_move_cost
static int ivm64_register_move_cost (machine_mode mode ATTRIBUTE_UNUSED,
                      reg_class_t from ATTRIBUTE_UNUSED, reg_class_t to)
{
     if (to == TR_REG) {return 100000;}
     return 8;
}

#undef TARGET_MEMORY_MOVE_COST
#define TARGET_MEMORY_MOVE_COST ivm64_memory_move_cost
static int ivm64_memory_move_cost (machine_mode mode ATTRIBUTE_UNUSED,
             reg_class_t rclass ATTRIBUTE_UNUSED, bool in ATTRIBUTE_UNUSED)
{
    return 9;
}

#undef TARGET_ADDRESS_COST
#define TARGET_ADDRESS_COST ivm64_address_cost
static int ivm64_address_cost (rtx address, machine_mode mode ATTRIBUTE_UNUSED,
                               addr_space_t as ATTRIBUTE_UNUSED, bool speed ATTRIBUTE_UNUSED)
{
     return (CONSTANT_ADDRESS_P(address) ? 1 : STACK_RELATIVE_P(address) ? 3 : 4);
}
#define IVM64_COSTS_N_INSNS(N)  (N)

// Compute cost of operations
static int ivm64_op_cost(int code)
{
    switch (code) {
        case PLUS:      return IVM64_COSTS_N_INSNS(1);
        case MULT:      return IVM64_COSTS_N_INSNS(1);
        case NOT:       return IVM64_COSTS_N_INSNS(1);
        case AND:       return IVM64_COSTS_N_INSNS(1);
        case IOR:       return IVM64_COSTS_N_INSNS(1);
        case XOR:       return IVM64_COSTS_N_INSNS(1);
        case LTU:       return IVM64_COSTS_N_INSNS(1);

        case UDIV:      return IVM64_COSTS_N_INSNS(1);
        case UMOD:      return IVM64_COSTS_N_INSNS(1);
        case DIV:       return IVM64_COSTS_N_INSNS(54);
        case MOD:       return IVM64_COSTS_N_INSNS(43);

        case ASHIFTRT:  return IVM64_COSTS_N_INSNS(1) + ivm64_op_cost(DIV);
        case LSHIFTRT:  return IVM64_COSTS_N_INSNS(1) + ivm64_op_cost(UDIV);
        case ASHIFT:    return IVM64_COSTS_N_INSNS(1) + ivm64_op_cost(MULT);

        case MINUS:     return ivm64_op_cost(NEG) + ivm64_op_cost(PLUS);
        case NEG:       return IVM64_COSTS_N_INSNS(2) + ivm64_op_cost(MULT);

        case GT:        return ivm64_op_cost(LT);
        case LT:        return ivm64_op_cost(LTU) + 2 * (IVM64_COSTS_N_INSNS(1) +
                                IVM64_COSTS_N_INSNS(1) + ivm64_op_cost(PLUS));
        case GE:        return ivm64_op_cost(LT) + ivm64_op_cost(NOT);
        case LE:        return ivm64_op_cost(LT) + ivm64_op_cost(NOT);
        case GTU:       return ivm64_op_cost(LTU);
        case GEU:       return ivm64_op_cost(LTU) + ivm64_op_cost(NOT);
        case LEU:       return ivm64_op_cost(LTU) + ivm64_op_cost(NOT);
        case EQ:        return ivm64_op_cost(LTU) + ivm64_op_cost(XOR) +
                                IVM64_COSTS_N_INSNS(1);
        case NE:        return ivm64_op_cost(LTU) + ivm64_op_cost(XOR) +
                                IVM64_COSTS_N_INSNS(1) + ivm64_op_cost(NOT);
        case SIGN_EXTEND:
                        return IVM64_COSTS_N_INSNS(6);
        case ZERO_EXTEND:
                        return IVM64_COSTS_N_INSNS(0);
        default:
                        return IVM64_COSTS_N_INSNS(1);
    }
}

/* True if RTX x is an UNSPEC pattern,
   providing its unspec type in unspec_code */
static bool ivm64_unspec_p(rtx x, int *unspec_code){
    RTX_CODE code = GET_CODE(x);
    if ( code == UNSPEC || code == UNSPEC_VOLATILE) {
        *unspec_code = XINT(x,1);
        return true;
    } else {
        *unspec_code = 0;
        return false;
    }
}

static int ivm64_rtx_basic_cost(rtx x, machine_mode mode, int outer_code, int opno, bool speed)
{
    RTX_CODE code = GET_CODE(x);


    // Isolated basic RTXs: push(x) or pop (x)
    if (CONST_INT_P(x) || CONST_WIDE_INT_P(x) || CONST_FIXED_P(x) || CONST_DOUBLE_P(x)
                       || GET_CODE(x) == LABEL_REF || GET_CODE(x) == SYMBOL_REF)
    {
        return 0;
    }


    if (SUBREG_P(x)){
        return ivm64_rtx_basic_cost(SUBREG_REG(x), mode, outer_code, opno, speed);
    }

    if (GET_CODE(x) == CONST) {
        /* Patterns like (const (plus (...)) */
        return ivm64_rtx_basic_cost(XEXP(x,0), mode, outer_code, opno, speed);
    }

    if (REG_P(x)){
        // GPRs / pseudoregisters
        if (IS_GP_REGNUM(REGNO(x)) || REGNO(x) >= FIRST_PSEUDO_REGISTER) {
            // push_sp + push offset + add + (load/store)
            return 3;
        }

        if (REGNO(x) == TR_REGNUM) {
            if (outer_code == SET && opno == 1){
                return 1000;
            }
            return 0;
        }

        return 1;
    }

    if (code == MEM) {
        rtx addr = XEXP(x,0);
        if (GET_CODE(addr) == PRE_DEC){
            return 0;
        }
        if (CONSTANT_P(addr) || SUM_OF_CONST_P(addr)) {
            return 1;
        }
        if (PC_OR_STACK_RELATIVE_P(addr)) {
            return 3;
        }
        if (MEM_INDIRECT_P(addr)) {
            return 5;
        }

        return 8;
    }

    int unspec_code;
    if (ivm64_unspec_p(x, &unspec_code)){
        if (unspec_code == UNSPEC_PUSH_TR) {
            rtx unspec_op = XVECEXP (x, 0, 1);
            return ivm64_rtx_basic_cost(unspec_op, mode, outer_code, opno, speed);
        }
        if (unspec_code == UNSPEC_POP_TR) {
           return 0;
        }
    }

    if (UNARY_P(x)) {
        return ivm64_op_cost(code) + ivm64_rtx_basic_cost(XEXP(x,0), mode, code, 0, speed);
    }

    if (BINARY_P(x)) {
        return ivm64_op_cost(code)
               + ivm64_rtx_basic_cost(XEXP(x,0), mode, code, 0, speed)
               + ivm64_rtx_basic_cost(XEXP(x,1), mode, code, 0, speed);
    }

    if (code == IF_THEN_ELSE) {
        return ivm64_rtx_basic_cost(XEXP(x,0), mode, code, 0, speed);
    }

    if (ordered_comparison_operator(x, mode)) {
        return ivm64_op_cost(code)
               + ivm64_rtx_basic_cost(XEXP(x,0), mode, code, 0, speed)
               + ivm64_rtx_basic_cost(XEXP(x,1), mode, code, 0, speed);
    }

    if (code == SET) {
        rtx dst = SET_DEST(x);
        rtx src = SET_SRC(x);

        return ivm64_rtx_basic_cost(src, mode, code, 0, speed)
               + ivm64_rtx_basic_cost(dst, mode, code, 1, speed);
    }

    return 1;
}

#undef TARGET_RTX_COSTS
#define TARGET_RTX_COSTS ivm64_rtx_costs
static bool ivm64_rtx_costs(rtx x, machine_mode mode, int outer_code,
                 int opno, int *total, bool speed)
{
    *total = ivm64_rtx_basic_cost(x, mode, outer_code, opno, speed);
    return true;
}



// Used with -O1, -O2, -dp or -dP compiler option (not called with -O0)
#undef TARGET_INSN_COST
#define TARGET_INSN_COST ivm64_insn_cost
static int ivm64_insn_cost(rtx_insn *insn, bool speed)
{
    if (JUMP_P(insn)) {
        rtx set = single_set(insn); // DEST -> pc
        if (set == NULL) { // asm...
            return 0;
        }
        rtx src_rtx = SET_SRC(set);
        if (GET_CODE(src_rtx) == IF_THEN_ELSE) { // conditional
            rtx cond_rtx = XEXP(src_rtx, 0);
            int opcode = GET_CODE(cond_rtx);
            int cost0 = 0;
            int cost1 = 0;
            rtx oper0 = XEXP(cond_rtx, 0);
            rtx oper1 = XEXP(cond_rtx, 1);
            ivm64_rtx_costs(oper0, DImode, opcode, 0, &cost0, speed);
            ivm64_rtx_costs(oper1, DImode, opcode, 1, &cost1, speed);
            return IVM64_COSTS_N_INSNS(1) + ivm64_op_cost(opcode) + cost0 + cost1;
        } else { // unconditional
            int cost = 0;
            ivm64_rtx_costs(src_rtx, DImode, SET, 1, &cost, speed);
            return IVM64_COSTS_N_INSNS(4) + cost;
        }
    } else if (CALL_P(insn)) {
        int cost0 = 0;
        rtx address = XEXP(XEXP(insn, 0), 0);
        ivm64_rtx_costs(address, DImode, CALL, 0, &cost0, speed);
        //  push RA + compute(dest) + push dest + jump + store RESULT + set_sp
        return IVM64_COSTS_N_INSNS(3+4+4) + cost0;
    } else if (INSN_P(insn)) {
        rtx set = single_set(insn);
        if (set == NULL) { // use, asm, clobber...
            return 0;
        }
        rtx dest = SET_DEST(set);
        rtx src = SET_SRC(set);
        int cost1 = 0;
        ivm64_rtx_costs(src, GET_MODE(src), SET, 1, &cost1, speed);
        if (MEM_P(dest) && GET_CODE(XEXP(dest, 0)) == PRE_DEC &&
            REGNO(XEXP(XEXP(dest, 0), 0)) == STACK_POINTER_REGNUM) { // push
            return cost1;
        } else { // move
            int cost0 = 0;
            ivm64_rtx_costs(dest, GET_MODE(dest), SET, 0, &cost0, speed);
            return IVM64_COSTS_N_INSNS(1) + cost0 + cost1;
        }

    } else {
    }
    return IVM64_COSTS_N_INSNS(0);
}


/* Output a string based on name, suitable for the ‘#ident’ directive */
void ivm64_asm_output_ident(const char *ident_str)
{
    const char *ident_asm_op = "#\t.ident\t";
    fprintf (asm_out_file, "%s\"%s\"\n", ident_asm_op, ident_str);
}


/* Print an integer */
static bool ivm64_asm_integer(rtx x, unsigned int size, int aligned_p ATTRIBUTE_UNUSED)
{
    FILE *file = asm_out_file;

    if (size > UNITS_PER_WORD) {
        return 0;
    }

    switch (size){
        case 1:
            fputs ("\tdata1 " "[", file);
            break;
        case 2:
            fputs ("\tdata2 " "[", file);
            break;
        case 4:
            fputs ("\tdata4 " "[", file);
            break;
        case 8:
            fputs ("\tdata8 " "[", file);
            break;
        default:
            /* No data directive for this size,
               let gcc split it into small pieces */
            return 0;
    }

    ivm64_output_constant(file, x);
    fputs ("]\n", file);
    return 1;
}

void ivm64_output_data_zero_vector(FILE *file, unsigned long size)
{
    unsigned long s8 = size/8;
    unsigned long s4 = (size % 8)/4;
    unsigned long s2 = ((size % 8) % 4)/2;
    unsigned long s1 = (((size % 8) % 4) % 2);

    if (s8 > 1)
        fprintf(file, "\tdata8 [0]*%ld\n", s8);
    else if (s8 == 1)
        fprintf(file, "\tdata8 [0]\n");

    if (s4)
        fprintf(file, "\tdata4 [0]\n");

    if (s2)
        fprintf(file, "\tdata2 [0]\n");

    if (s1)
        fprintf(file, "\tdata1 [0]\n");

    fprintf(file, "\n");
}

/*  Used to define macros ASM_OUTPUT_COMMON and ASM_OUTPUT_LOCAL*/
void ivm64_initialize_uninitialized_data(FILE *file, const char *name,
             unsigned long size, int rounded ATTRIBUTE_UNUSED, bool global)
{
    if (in_section == text_section)
        switch_to_section (data_section);
    if (global)
        TARGET_ASM_GLOBALIZE_LABEL(file, name);

    assemble_name (file, name);
    fputs(":\n", file);
    ivm64_output_data_zero_vector(file, size);
}

/* Implement PUSH_ROUNDING */
poly_int64 ivm64_push_rounding(poly_int64 bytes)
{
    return (bytes + UNITS_PER_WORD -1) & ~(UNITS_PER_WORD -1);
}

/* Return an RTX representing where to find the return value of a
   function returning MODE. */
static rtx ivm64_rtx_function_value(machine_mode mode)
{
   /* Use AR for the return value; note that for 2-word return
      values the next hard register (X1) is used for the most significand
      word */
   return gen_rtx_REG(mode, AR_REGNUM);
}

/* Target hook for TARGET_FUNCTION_VALUE */
static rtx ivm64_function_value (const_tree valtype,
               const_tree fn_decl_or_type ATTRIBUTE_UNUSED,
               bool outgoing)
{
    rtx ret;
    machine_mode mode;
    mode = TYPE_MODE(valtype);
    if (cfun && cfun->machine){ // This function can be called outside a function (cfun=NULL),
                                // e.g., ivm64-gcc pr43879_1.c -O2 -fipa-pta
        cfun->machine->volatile_ret = cfun->machine->volatile_ret?
                                         (cfun->machine->volatile_ret):
                                         (outgoing && TREE_THIS_VOLATILE(valtype));
    }
    if (outgoing && RETURN_ON_ARGS){
         ret = gen_rtx_MEM(mode, gen_rtx_PLUS(DImode, arg_pointer_rtx, GEN_INT(0)));
         MEM_VOLATILE_P(ret) = 1;
    } else {
         ret = ivm64_rtx_function_value(mode);
    }
    return ret;
}

/* Used to define macro LIBCALL_VALUE */
rtx ivm64_libcall_value (machine_mode mode)
{
    return ivm64_rtx_function_value(mode);
}

/* Target hook for TARGET_FUNCTION_VALUE_REGNO_P, a target hook that returns
   true if regno is the number of a hard register in which the values of called
   function may come back.*/
static bool ivm64_function_value_regno_p(const unsigned int regno)
{
    return (regno == AR_REGNUM) || (regno == X1_REGNUM) ;
}

/* Target hook for TARGET_LEGITIMATE_ADDRESS_P */
bool ivm64_legitimate_address_p(machine_mode mode ATTRIBUTE_UNUSED, rtx x,
                                bool strict ATTRIBUTE_UNUSED)
{
    return PC_OR_STACK_RELATIVE_P(x) || (MEM_INDIRECT_RELATIVE_P(x));
}



rtx ivm64_return_addr(int count, rtx frame ATTRIBUTE_UNUSED)
{
    if (count != 0)
        return NULL_RTX;

    rtx ret = gen_rtx_MEM(DImode,
                          gen_rtx_PLUS(DImode, arg_pointer_rtx,
                                  GEN_INT(-UNITS_PER_WORD)));
    return ret;
}

/* Defining data structures for per-function information. */

// Function to init struct machine_function.
static struct machine_function * ivm64_init_machine_status(void)
{
    return ggc_cleared_alloc<machine_function>();
}

// Used to define macro INIT_EXPANDERS (see ivm64.h)
void ivm64_init_expanders(void)
{
    init_machine_status = ivm64_init_machine_status;
}

#undef TARGET_SET_CURRENT_FUNCTION
#define TARGET_SET_CURRENT_FUNCTION ivm64_set_current_function
void ivm64_set_current_function(tree decl)
{
    if (decl == NULL_TREE
      || current_function_decl == NULL_TREE
      || current_function_decl == error_mark_node
      || ! cfun->machine)
    return;
    cfun->machine->volatile_ret = false;
}

// Return true if a volatile type return has been registered
// for the current function
static bool ivm64_get_cfun_volatil_ret(){
    if (cfun)
        if (cfun->machine)
            return cfun->machine->volatile_ret;
    return false;
}


/* Initialize the GCC target structure.  */

/* These hooks specify assembly directives for creating certain kinds of
   integer object. The TARGET_ASM_BYTE_OP directive creates a byte-sized
   object, the TARGET_ASM_ALIGNED_HI_OP one creates an aligned
   two-byte object, and so on. Any of the hooks may be NULL, indicating
   that no suitable directive is available. */
/* Defined as NULL to force to use TARGET_ASM_INTEGER */
#undef TARGET_ASM_BYTE_OP
#define TARGET_ASM_BYTE_OP NULL

#undef TARGET_ASM_ALIGNED_HI_OP
#define TARGET_ASM_ALIGNED_HI_OP    NULL
#undef TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP    NULL
#undef TARGET_ASM_ALIGNED_DI_OP
#define TARGET_ASM_ALIGNED_DI_OP    NULL
#undef TARGET_ASM_UNALIGNED_HI_OP
#define TARGET_ASM_UNALIGNED_HI_OP  NULL
#undef TARGET_ASM_UNALIGNED_SI_OP
#define TARGET_ASM_UNALIGNED_SI_OP  NULL
#undef TARGET_ASM_UNALIGNED_DI_OP
#define TARGET_ASM_UNALIGNED_DI_OP  NULL

/* The assemble_integer function uses this hook to output an integer object.
   The function should return true if it was able to output the object.
   If it returns false, assemble_integer will try to split the object
   into smaller parts. The default implementation of this hook will use
   the TARGET_ASM_BYTE_OP family of strings, returning false when the relevant
   string is NULL. */
#undef TARGET_ASM_INTEGER
#define TARGET_ASM_INTEGER ivm64_asm_integer

/* Define the parentheses used to group arithmetic operations
   in assembler code. */
#undef TARGET_ASM_OPEN_PAREN
#define TARGET_ASM_OPEN_PAREN  "("
#undef TARGET_ASM_CLOSE_PAREN
#define TARGET_ASM_CLOSE_PAREN ")"

#undef TARGET_ASM_FILE_START
#define TARGET_ASM_FILE_START ivm64_asm_file_start

#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG ivm64_function_arg

#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE  ivm64_function_arg_advance

#undef TARGET_LEGITIMIZE_ADDRESS
#define TARGET_LEGITIMIZE_ADDRESS ivm64_legitimize_address

#undef TARGET_RETURN_POPS_ARGS
#define TARGET_RETURN_POPS_ARGS ivm64_return_pops_args

#undef TARGET_HARD_REGNO_NREGS
#define TARGET_HARD_REGNO_NREGS ivm64_hard_regno_nregs

#undef TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK ivm64_hard_regno_mode_ok

#undef TARGET_MODES_TIEABLE_P
#define TARGET_MODES_TIEABLE_P ivm64_modes_tieable_p

#undef TARGET_OPTION_OVERRIDE
#define TARGET_OPTION_OVERRIDE ivm64_option_override

#undef TARGET_STARTING_FRAME_OFFSET
#define TARGET_STARTING_FRAME_OFFSET ivm64_starting_frame_offset

#undef TARGET_ASM_FUNCTION_PROLOGUE
#define TARGET_ASM_FUNCTION_PROLOGUE ivm64_asm_function_prologue

#undef TARGET_ASM_FUNCTION_EPILOGUE
#define TARGET_ASM_FUNCTION_EPILOGUE ivm64_asm_function_epilogue

#undef TARGET_LRA_P
#define TARGET_LRA_P hook_bool_void_false

#undef TARGET_USE_BY_PIECES_INFRASTRUCTURE_P
#define TARGET_USE_BY_PIECES_INFRASTRUCTURE_P ivm64_use_by_pieces_infrastructure_p

#undef TARGET_CASE_VALUES_THRESHOLD
#define TARGET_CASE_VALUES_THRESHOLD ivm64_case_values_threshold

#undef TARGET_ASM_OUTPUT_IDENT
#define TARGET_ASM_OUTPUT_IDENT ivm64_asm_output_ident

/* Enable this to disable inline completely. */
#if (IVM64_DISABLE_INLINE == 1)
    #undef TARGET_CAN_INLINE_P
    #define TARGET_CAN_INLINE_P ivm64_can_inline_p
#endif

#undef CTORS_SECTION_ASM_OP
#define CTORS_SECTION_ASM_OP "#.ctors"

#undef DTORS_SECTION_ASM_OP
#define DTORS_SECTION_ASM_OP "#.dtors"

#undef TARGET_ASM_CONSTRUCTOR
#define TARGET_ASM_CONSTRUCTOR ivm64_asm_out_ctor

#undef TARGET_ASM_DESTRUCTOR
#define TARGET_ASM_DESTRUCTOR ivm64_asm_out_dtor

#undef TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE ivm64_function_value

#undef TARGET_FUNCTION_VALUE_REGNO_P
#define TARGET_FUNCTION_VALUE_REGNO_P ivm64_function_value_regno_p

#if (IVM64_NON_BUILTIN_ALLOCA == 1)
    /* Use alloca function instead of the builtin alloca */
    #undef TARGET_HAVE_ALLOCATE_STACK
    #define TARGET_HAVE_ALLOCATE_STACK     hook_bool_void_true
    #undef TARGET_CODE_FOR_ALLOCATE_STACK
    #define TARGET_CODE_FOR_ALLOCATE_STACK CODE_FOR_call_alloca
#endif

#define TARGET_FUNCTION_OK_FOR_SIBCALL hook_bool_tree_tree_false


static rtx ivm64_static_chain(const_tree fndecl_or_type ATTRIBUTE_UNUSED,
                              bool incoming_p ATTRIBUTE_UNUSED)
{
  rtx mem;
  mem = gen_rtx_MEM(DImode, gen_rtx_SYMBOL_REF(Pmode, ".XSC"));
  MEM_NOTRAP_P (mem) = 1;
  MEM_VOLATILE_P(mem) = 1;
  return mem;
}

/* Generate assembler code for constant parts of a trampoline. */
static void ivm64_asm_trampoline_template(FILE *f)
{
    fprintf(f, "\tpush! 80000000021\n");   // Jump address
    fprintf(f, "\tpush! 81000000077\n");   // Static chain value
    fprintf(f, "\tpush! 82000000127\n");   // Static chain register (.XSC)
    fprintf(f, "\tstore8 \n");
    fprintf(f, "\tjump\n");
    fprintf(f, "\tdata1 [0]*3\n");
    fprintf(f, "\n\n");
}


/* Emit RTL insns to initialize the variable parts of a trampoline. */
static void ivm64_trampoline_init(rtx m_tramp, tree fndecl, rtx chain_value)
{
  rtx fnaddr = XEXP(DECL_RTL(fndecl), 0);

  // Copy the trampoline
  emit_block_move (m_tramp, assemble_trampoline_template(),
                   GEN_INT(TRAMPOLINE_SIZE), BLOCK_OP_NORMAL);

  rtx mem1 = adjust_address(m_tramp, DImode, 1);
  ivm64_expand_push(fnaddr, DImode);
  ivm64_expand_pop(mem1, DImode);

  rtx mem2 = adjust_address(m_tramp, DImode, 10);
  ivm64_expand_push(chain_value, DImode);
  ivm64_expand_pop(mem2, DImode);

  rtx screg = gen_rtx_SYMBOL_REF(DImode, ".XSC");
  rtx mem3 = adjust_address(m_tramp, DImode, 19);
  ivm64_expand_push(screg, DImode);
  ivm64_expand_pop(mem3, DImode);
}

#undef TARGET_STATIC_CHAIN
#define TARGET_STATIC_CHAIN ivm64_static_chain
#undef TARGET_ASM_TRAMPOLINE_TEMPLATE
#define TARGET_ASM_TRAMPOLINE_TEMPLATE ivm64_asm_trampoline_template
#undef TARGET_TRAMPOLINE_INIT
#define TARGET_TRAMPOLINE_INIT ivm64_trampoline_init


struct gcc_target targetm = TARGET_INITIALIZER;

#include "gt-ivm64.h"  /* Why does not ivm64 generate gt-ivm64.h? */
