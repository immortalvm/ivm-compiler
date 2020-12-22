/*
 * Preservation Virtual Machine Project
 * Helper functions for the ivm64 target
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: Oct 2019 - Dec 2020
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
#include "params.h"
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

/* Number of extra peephole2 passes*/
static int ivm64_extra_peep2_count = 0;

/* Ids of extra peephole2 passes*/
static int ivm64_extra_peep2_pass_number[8];

/* This variable contains a pointer to the operand used as
   destination in the last operand transfer */
rtx ivm64_last_set_operand = 0;

/* An alias for generating TR_REGNUM register rtx*/
#define TR_REG_RTX(mode) (gen_rtx_REG((mode), TR_REGNUM))

/* Run-time Target Specification.  */
void ivm64_cpu_cpp_builtins(struct cpp_reader *pfile)
{
    #define builtin_assert(TXT) cpp_assert (pfile, TXT)
    #define builtin_define(TXT) cpp_define (pfile, TXT)
    builtin_define ("__ivm64__");
}


/* To enable/disable individual peehole patterns
   Return 1 if a peephole is enabled, 0 otherwise  */
#define IVM64_SET_PEEP(peep2, enable) case peep2: return enable; break;
int ivm64_peep_enabled(enum ivm64_peephole2 peep)
{
    switch(peep) {
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP,          1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP_PUSH_POP, 1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_MOVE,         1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH_POP_PUSH, 1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_MOVE,          1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_MOVE2,         1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSHARG,       1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSHARG,      1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_BINOP,                 1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP1,                     1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP_1_2_3,                1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP_1_2,                  1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP_1_DEADREG_BINOP_2,    1);
        IVM64_SET_PEEP(IVM64_PEEP2_BINOP_1_2_DEADREG_BINOP_3,  1);
        IVM64_SET_PEEP(IVM64_PEEP2_ZERO_EXTEND,                1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP_DEADREG_BINOP,          1);
        IVM64_SET_PEEP(IVM64_PEEP2_UNARY_DEADREG_BINOP,             1);
        IVM64_SET_PEEP(IVM64_PEEP2_UNARY_1_BINOP_2_DEADREG_BINOP_3, 1);
        IVM64_SET_PEEP(IVM64_PEEP2_RDX_1_2,                         1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_BLOCK_PUSH_BLOCK, 1);
        IVM64_SET_PEEP(IVM64_PEEP2_SIGNX,                1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSH_COMMUTATIVE,      1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSH_IMM_COMMUTATIVE,  1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH_SUB,               1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSH_SUB,              1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH_COMMUTATIVE,       1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_BLOCK_PUSH_COMMUTATIVE, 1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_PUSH,                  1);
        IVM64_SET_PEEP(IVM64_PEEP2_POW2,                       1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE,                   1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSH,               1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_PUSHDI,             1);
        IVM64_SET_PEEP(IVM64_PEEP2_POPDI_PUSH,             1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_IND_PUSH,           1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_IND_OFFSET_PUSH,    1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_CBRANCH,            1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_CBRANCH_REV,        1);
        IVM64_SET_PEEP(IVM64_PEEP2_POPDI_BLOCK_CBRANCH,    1);
        IVM64_SET_PEEP(IVM64_PEEP2_POPDI_CBRANCH,          1);
        IVM64_SET_PEEP(IVM64_PEEP2_POPDI_CBRANCH_REV,      1);
        IVM64_SET_PEEP(IVM64_PEEP2_SET_CBRANCH,            1);
        IVM64_SET_PEEP(IVM64_PEEP2_SET_CBRANCH_REV,        1);
        IVM64_SET_PEEP(IVM64_PEEP2_SIGNX_POP_CBRANCH_REV,  1);
        IVM64_SET_PEEP(IVM64_PEEP2_BLOCK_BLOCK,   1);
        IVM64_SET_PEEP(IVM64_PEEP2_SET_NOP,       1);
        IVM64_SET_PEEP(IVM64_PEEP2_NOP_SET,       1);
        IVM64_SET_PEEP(IVM64_PEEP2_POP_NOP_PUSH,  1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOV_PUSH,      1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOV_PUSH_POP_PUSH_POP, 1);
        IVM64_SET_PEEP(IVM64_PEEP2_CALL_PUSH_AR,        1);
        IVM64_SET_PEEP(IVM64_PEEP2_BLOCK_POP_BLOCK,               1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_BINOP,                    1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_POP_PUSH_BINOP_MULTIMODE, 1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_INDPUSH_ZERO_EXTEND,      1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_SUB_NEG,                  1);
        IVM64_SET_PEEP(IVM64_PEEP2_AND_POP,                       1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_ADD_POP_PUSH_IND_POP,       1);
        IVM64_SET_PEEP(IVM64_PEEP2_PUSH_ADD_POP_PUSH_IND_OFFSET_POP,1);
        IVM64_SET_PEEP(IVM64_PEEP2_MOVE_MOVE_MOVE,                  1);
        // assembly peepholes
        IVM64_SET_PEEP(IVM64_PEEP1_MOVE_PUSH_POP,          1);
        IVM64_SET_PEEP(IVM64_PEEP1_SHIFTRU63_AND,          1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSHAR_BINOP_POPAR,     1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_COMMUTATIVE_POPAR, 1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_SUB_POPAR,         1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_BINOP_BINOP_POPAR, 1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_ADD,               1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSHAR_UNARY_POPAR,     1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSHAR_SIGNX_POPAR,     1);
        IVM64_SET_PEEP(IVM64_PEEP1_POP_PUSH,               1);
        IVM64_SET_PEEP(IVM64_PEEP1_PUSH_SIGNX,             1);
        IVM64_SET_PEEP(IVM64_PEEP1_IND_PUSH_SIGNX,         1);
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
void ivm64_fatal (const char *msg)
{
    fprintf (stderr, "%s: Internal IVM64-GCC abort.\n%s\n", main_input_filename, msg);
    exit (FATAL_EXIT_CODE);
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

int ivm64_eliminable_register (rtx x)
{
  return REG_P (x) && (REGNO (x) == FRAME_POINTER_REGNUM);
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

/* In a call instruction, return if the return value (AR) is
   unused*/
int return_value_unused(rtx call_insn){
    if (find_regno_note (call_insn, REG_UNUSED, AR_REGNUM) != NULL_RTX) {
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


/* EXPAND FUNCTIONS */

void ivm64_init_expanders()
{
}

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
    emit_insn(gen_blockage());
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
#define IVM64_DATA_SMALL_ASCII_LEN 64
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
        if (REGNO(x) >=  (FIRST_GP_REGNUM + used_gp_regs())) {
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

    output_asm_insn (ispush ? "load8! %0" : "load8! %0", &address);

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

/*  Output instructions for moving the return value after a function call returning
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
                rtx args_nbytes_rtx8 = GEN_INT(args_nbytes / UNITS_PER_WORD); // stack units
                output_asm_insn("store8! &%0", &args_nbytes_rtx8);
            }
        }

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
        output_asm_insn("push! $1", NULL);
        output_asm_insn("store8! &3", NULL);
        output_asm_insn("store8! &1", NULL);
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
void ivm64_output_cbranch_multimode(rtx *operands, machine_mode mode)
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

    rtx rtx_constant_0 = CONST0_RTX (mode);
    if (((NE == code) || (EQ == code))
        && (rtx_equal_p (operands[1], rtx_constant_0)
            || rtx_equal_p (operands[2], rtx_constant_0))
    ) {

        /* Stack adjust not necessary as this push is swallowed by the
           jump; no comparison emitted when comparing with 0 */
        if (rtx_equal_p (operands[1], rtx_constant_0))
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
    } else if (EQ == code) {
        ivm64_output_push(operand0, GET_MODE(operand0));
        ivm64_stack_extra_offset += UNITS_PER_WORD;

        ivm64_output_push(operand1, GET_MODE(operand1));

        output_asm_insn("xor", NULL);
        ivm64_stack_extra_offset -= UNITS_PER_WORD;

        output_asm_insn("jump_zero! %3", operands);
        return;
    } else {
        rtx const2N_1 = GEN_INT((-1UL) << (8*modesize-1));

        if (inskind==0) { // signed lt_s -> lt_u
            ivm64_output_push(operand0, GET_MODE(operand0));
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            output_asm_insn("add! %0", &const2N_1);

            ivm64_output_push(operand1, GET_MODE(operand1));
            output_asm_insn("add! %0", &const2N_1);
        } else if (inskind==1) { // native lt_u
            ivm64_output_push(operand0, GET_MODE(operand0));
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            ivm64_output_push(operand1, GET_MODE(operand1));
        } else {    // eq/ne
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


/* Print a compare and branch action for one operand in the best possible way
   to reduce the number of native ivm64 instructions; the first operand
   is already on the stack. To be used in peepholes. */
void ivm64_output_cbranch_multimode_peep(rtx *operands, machine_mode mode, int reverse)
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


    /* Adapt integer constants to the size of the mode (zero_extend) */
    operand1 = ivm64_const_int_zero_extend(operand1, modesize);

    /* Let's zero extend the operand already in the stack
       (i.e., this one supressed by the peephole) */
    rtx const0extend = GEN_INT(mask0extend);
    if (modesize < 8){
        output_asm_insn("and! %0", &const0extend);
    }

    rtx rtx_constant_0 = CONST0_RTX (DImode);
    if (((NE == code) || (EQ == code))
        && (rtx_equal_p (operand1, rtx_constant_0))
    ) {

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
        rtx const2N_1 = GEN_INT((-1UL) << (8*modesize-1));

        if (inskind==0) { // signed lt_s -> lt_u
            ivm64_stack_extra_offset += UNITS_PER_WORD;
            output_asm_insn("add! %0", &const2N_1);

            ivm64_output_push(operand1, GET_MODE(operand1));
            output_asm_insn("add! %0", &const2N_1);
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
    int min_idx_int = -INTVAL(operands[1]);
    rtx min_idx_rtx = GEN_INT(min_idx_int);


    /* (unsigned)(index - lower_bound) > range */
    ivm64_output_push(operands[2], SImode);
    ivm64_stack_extra_offset += UNITS_PER_WORD;

    ivm64_output_push(operands[0], SImode);
    ivm64_stack_extra_offset += UNITS_PER_WORD;
    // sign extend
    fprintf(asm_out_file, "\txor! %ld\n", 0x080000000UL);
    fprintf(asm_out_file, "\tadd! %ld\n", 0xffffffff80000000);

    if (min_idx_int != 0) {
        output_asm_insn("add! %0", &min_idx_rtx);
    }
    output_asm_insn("lt_u", NULL);
    ivm64_stack_extra_offset -= UNITS_PER_WORD;

    output_asm_insn("not", NULL);

    /* jump to the out-of-range label*/
    output_asm_insn("jump_zero! %4", operands);
    ivm64_stack_extra_offset -= UNITS_PER_WORD;

    rtx word_size = GEN_INT(UNITS_PER_WORD);

    /* compute (index - lower bound)*sizeof(long)
       to index the dispatch table*/
    ivm64_output_push(operands[0], SImode);
    ivm64_stack_extra_offset += UNITS_PER_WORD;
    // sign extend
    fprintf(asm_out_file, "\txor! %ld\n", 0x080000000UL);
    fprintf(asm_out_file, "\tadd! %ld\n", 0xffffffff80000000);

    if (min_idx_int != 0) {
        output_asm_insn("add! %0", &min_idx_rtx);
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

    #if (IVM64_DISABLE_X_FLAG_TREE_TER == 1)
        global_options.x_flag_tree_ter = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_DELAYED_BRANCH == 1)
        global_options.x_flag_delayed_branch = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_DSE == 1)
        global_options.x_flag_dse = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_CROSSJUMPING == 1)
        global_options.x_flag_crossjumping = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_DELETE_NULL_POINTER_CHECKS == 1)
        global_options.x_flag_delete_null_pointer_checks = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_CPROP_REGISTERS == 1)
        global_options.x_flag_cprop_registers = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_CSE_FOLLOW_JUMPS == 1)
        global_options.x_flag_cse_follow_jumps = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_CSE_AFTER_LOOP == 1)
        global_options.x_flag_rerun_cse_after_loop= 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_IVOPTS == 1)
        global_options.x_flag_ivopts = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_TREE_CH == 1)
        global_options.x_flag_tree_ch = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_TREE_LOOP_OPTIMIZE == 1)
        global_options.x_flag_tree_loop_optimize = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_ISOLATE_ERRONEOUS_PATHS_DEREFERENCE == 1)
        global_options.x_flag_isolate_erroneous_paths_dereference = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_TREE_DCE == 1)
        global_options.x_flag_tree_dce = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_CODE_HOISTING == 1)
            global_options.x_flag_code_hoisting = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_TREE_PRE == 1)
            global_options.x_flag_tree_pre = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_EARLY_INLINING == 1)
            global_options.x_flag_early_inlining = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_INLINE_SMALL_FUNCTIONS == 1)
            global_options.x_flag_inline_small_functions= 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_INLINE  == 1)
            global_options.x_flag_no_inline= 1; /* This flag is inverted */
    #endif
    #if (IVM64_DISABLE_X_FLAG_TREE_SRA == 1)
            global_options.x_flag_tree_sra = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_DCE == 1)
            global_options.x_flag_dce= 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_TREE_FORWPROP == 1)
            global_options.x_flag_tree_forwprop = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_EXPENSIVE_OPTIMIZATIONS == 1)
            global_options.x_flag_expensive_optimizations = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_TREE_VECTORIZE == 1)
            global_options.x_flag_tree_loop_vectorize = 0;
            global_options.x_flag_tree_slp_vectorize = 0;
    #endif
    #if (IVM64_DISABLE_X_FLAG_FAST_MATH == 1)
            global_options.x_flag_unsafe_math_optimizations = 0;
            global_options.x_flag_finite_math_only= 0;
    #endif
    #if (IVM64_DISABLE_RTL_CSE1 == 1)
        disable_pass("rtl-cse1");
    #endif
    #if (IVM64_DISABLE_RTL_CSE2 == 1)
        disable_pass("rtl-cse2");
    #endif
    #if (IVM64_DISABLE_RTL_CPROP == 1)
        disable_pass("rtl-cprop1");
        disable_pass("rtl-cprop2");
        disable_pass("rtl-cprop3");
    #endif
    #if (IVM64_REORDER_BLOCKS_ALGORITHM_STC == 1)
        flag_reorder_blocks_algorithm = REORDER_BLOCKS_ALGORITHM_STC;
    #endif
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
                                machine_mode mode ATTRIBUTE_UNUSED,
                                const_tree type ATTRIBUTE_UNUSED,
                                bool named ATTRIBUTE_UNUSED)
{
    return; /* Do nothing*/
}

rtx ivm64_function_arg(cumulative_args_t ca ATTRIBUTE_UNUSED,
                       machine_mode mode ATTRIBUTE_UNUSED,
                       const_tree type ATTRIBUTE_UNUSED,
                       bool named ATTRIBUTE_UNUSED)
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
 * function pops on returning, or 0 if the function pops no arguments and the
 * caller must therefore pop them all after the function returns.  fundecl is a
 * C variable whose value is a tree node that describes the function in
 * question. Normally it is a node of type FUNCTION_DECL that describes the
 * declaration of the function. From this you can obtain the DECL_ATTRIBUTES of
 * the function.  funtype is a C variable whose value is a tree node that
 * describes the function in question. Normally it is a node of type
 * FUNCTION_TYPE that describes the data type of the function. From this it is
 * possible to obtain the data types of the value and arguments (if known).
 *
 * FUNCTION_TYPE used to represent the type of non-member functions and of
 * static member functions. The TREE_TYPE gives the return type of the
 * function. The TYPE_ ARG_TYPES are a TREE_LIST of the argument types. The
 * TREE_VALUE of each node in this list is the type of the corresponding
 * argument; the TREE_PURPOSE is an expression for the default argument value,
 * if any. If the last node in the list is void_list_node (a TREE_LIST node
 * whose TREE_VALUE is the void_type_ node), then functions of this type do not
 * take variable arguments. Otherwise, they do take a variable number of
 * arguments.  Note that in C (but not in C++) a function declared like void
 * f() is an unpro- totyped function taking a variable number of arguments; the
 * TYPE_ARG_TYPES of such a function will be NULL. */
poly_int64 ivm64_return_pops_args(tree fundecl ATTRIBUTE_UNUSED,
                                tree funtype ATTRIBUTE_UNUSED, poly_int64 size)
{
    /* This makes all functions behave as variadic. Patterns 'call_pop'
       and 'call_value_pop' will be in charge of popping the arguments after
       the call */
    return size;
}

static void ivm64_asm_function_prologue(FILE *file)
{
     long size;
     size = get_frame_size(); /*bytes*/

     /* Add the space needed for GPRs */
     size += used_gp_regs() * UNITS_PER_WORD;

     if (frame_pointer_needed){
         const char *msg = ASM_COMMENT_START "Frame pointer not eliminated";
         fputs(msg, file);
         ivm64_fatal(msg);
     }

    /* Print instructions to allocate space for local vars. and used GPRs */
    if (size <= 4 * UNITS_PER_WORD){
        for (int k=0; k < size/UNITS_PER_WORD; k++){
             fprintf(file, "\tpush! 0\n");
        }
    } else {
        ivm64_output_setsp(file, -size/UNITS_PER_WORD);
    }

    /* Reset offsets and counters*/
    ivm64_gpr_offset = 0;
    ivm64_stack_extra_offset = 0;
    emitted_push_cfun = 0;
    emitted_pop_cfun = 0;
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
        if (global_options.x_optimize == 0 || !global_options.x_flag_tree_coalesce_vars){
            if (1 == ret_regs){
                fprintf(file,  "\tstore8! &%ld\n", (long)(size8) + 2);
            } else {
                fprintf(file,  "\tstore8! &%ld\n", (long)(size8) + 3);
                fprintf(file,  "\tstore8! &%ld\n", (long)(size8) + 3);
            }
            ivm64_output_setsp(file, (long)size8); /* release (frame - ret_regs) */
        } else {
            ivm64_output_setsp(file, (long)(size8 + ret_regs)); /* release frame */
        }
    } else {
        /* no return value */
        ivm64_output_setsp(file, (long)size8); /* release frame */
    }

    fprintf(file, "\treturn\n");

    if (ivm64_gpr_offset != 0
        || ivm64_stack_extra_offset !=0
        || ( emitted_push_cfun != emitted_pop_cfun) )
    {
        char msg[1024];
        sprintf(msg,  "ivm64 error: in '%s' not balanced stack at epilogue\n", current_function_name());
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
}

static void ivm64_asm_out_dtor (rtx symbol ATTRIBUTE_UNUSED,
                                int priority ATTRIBUTE_UNUSED)
{
  fputs ("#\t.global __do_global_dtors\n", asm_out_file);
}


#undef TARGET_REGISTER_MOVE_COST
#define TARGET_REGISTER_MOVE_COST ivm64_register_move_cost
static int ivm64_register_move_cost (machine_mode mode ATTRIBUTE_UNUSED,
                      reg_class_t from ATTRIBUTE_UNUSED, reg_class_t to)
{
     if (to == TR_REG) {return 100000;}
     return 2;
}

#undef TARGET_MEMORY_MOVE_COST
#define TARGET_MEMORY_MOVE_COST ivm64_memory_move_cost
static int ivm64_memory_move_cost (machine_mode mode ATTRIBUTE_UNUSED,
             reg_class_t rclass ATTRIBUTE_UNUSED, bool in ATTRIBUTE_UNUSED)
{
    return 4;
}

#define IVM64_COSTS_N_INSNS(N)  (N)

// Compute cost of operations
static int ivm64_op_cost(int code)
{
    switch (code) {
        case PLUS:      return IVM64_COSTS_N_INSNS(1);
        case MULT:      return IVM64_COSTS_N_INSNS(1);
        case DIV:       return IVM64_COSTS_N_INSNS(1);
        case MOD:       return IVM64_COSTS_N_INSNS(1);
        case NOT:       return IVM64_COSTS_N_INSNS(1);
        case AND:       return IVM64_COSTS_N_INSNS(1);
        case IOR:       return IVM64_COSTS_N_INSNS(1);
        case XOR:       return IVM64_COSTS_N_INSNS(1);
        case LTU:       return IVM64_COSTS_N_INSNS(1);

        case MINUS:     return ivm64_op_cost(NEG) + ivm64_op_cost(PLUS);
        case UDIV:      return IVM64_COSTS_N_INSNS(7);
        case UMOD:      return IVM64_COSTS_N_INSNS(7);
        case NEG:       return IVM64_COSTS_N_INSNS(1) + ivm64_op_cost(MULT);
        case ASHIFTRT:  return IVM64_COSTS_N_INSNS(10);
        case LSHIFTRT:  return IVM64_COSTS_N_INSNS(10);
        case ASHIFT:    return IVM64_COSTS_N_INSNS(1) + ivm64_op_cost(MULT);
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
                        return IVM64_COSTS_N_INSNS(5);
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

#undef TARGET_RTX_COSTS
#define TARGET_RTX_COSTS ivm64_rtx_costs
bool ivm64_rtx_costs(rtx x, machine_mode mode, int outer_code ATTRIBUTE_UNUSED,
                 int opno ATTRIBUTE_UNUSED, int *total, bool speed)
{
    RTX_CODE code = GET_CODE(x);

    if (CONSTANT_P(x) || SUM_OF_CONST_P(x)) {
        *total = 0;
        return true;
    }

    if (REG_P(x)){
        if (IS_GP_REGNUM(REGNO(x)) || REGNO(x)>LAST_VIRTUAL_REGISTER) {
            *total = 2;
        } else if (REGNO(x) == TR_REGNUM) {
            *total = 1000;
        } else {
            *total = 1;
        }
        return true;
    }

    if (code == MEM) {
        rtx addr = XEXP(x,0);
        if (GET_CODE(addr) == PRE_DEC){
            *total = 1;
            return true;
        }
        if (CONSTANT_P(addr) || SUM_OF_CONST_P(addr)) {
            *total = 1;
            return true;
        }
        if (PC_OR_STACK_RELATIVE_P(addr)) {
            *total = 2;
            return true;
        }
        if (MEM_INDIRECT_P(addr)) {
            *total = 4;
            return true;
        }
        int total_addr = 1;
        ivm64_rtx_costs(addr, mode, code, 0, &total_addr, speed);
        *total = 2 + total_addr;
        return true;
    }

    if (code == SET) {
        rtx dest = SET_DEST(x);
        rtx src = SET_SRC(x);
        if (REG_P(dest) && REGNO(dest) == TR_REGNUM)  {
            int unspec_code;
            if ((ivm64_unspec_p(src, &unspec_code) && unspec_code == UNSPEC_PUSH_TR)
                || unary_operator(src, mode)
                || binary_operator(src, mode)
                || CONSTANT_P(src)
               ){
                ivm64_rtx_costs(src, mode, code, 0, total, speed);
                return true;
            }
            *total = 10000;
            return true;
        }
        if (REG_P(dest) && CONSTANT_P(src)){
            *total = 0;
            return true;
        }
        return false;
    }

    int unspec_code;
    if (ivm64_unspec_p(x, &unspec_code)){
        *total = 0;
        if (unspec_code == UNSPEC_PUSH_TR) {
            rtx unspec_op = XVECEXP (x, 0, 1);
            int total_0 = 1;
            ivm64_rtx_costs(unspec_op, mode, code, 0, &total_0, speed);
            *total += total_0;
        }
        if (unspec_code == UNSPEC_POP_TR) {
           *total += 0;
        }
        /* We assign no cost to the other unspecs (BLOCKAGE, ...) */
        return true;
    }

    if (unary_operator(x, mode)) {
        int total_0 = 1;
        ivm64_rtx_costs(XEXP(x,0),mode,code,0, &total_0, speed);
        *total = ivm64_op_cost(code) + 1;
        return true;
    }

    if (binary_operator(x, mode)) {
        int total_0 = 1, total_1 = 1;
        rtx op0 = XEXP(x,0);
        if (REG_P(op0) && REGNO(op0) == TR_REGNUM)  {
            total_0 = 0;
        } else {
            ivm64_rtx_costs(op0, mode, code, 0, &total_0, speed);
        }
        ivm64_rtx_costs(XEXP(x,1), mode, code, 0, &total_1, speed);
        *total = ivm64_op_cost(code)*4 + total_0 + total_1;
        return true;
    }

    if (code == IF_THEN_ELSE) {
        ivm64_rtx_costs(XEXP(x,0), mode, code, 0, total, speed);
        return true;
    }

    if (ordered_comparison_operator(x, mode)) {
        int total_0 = 1, total_1 = 1;
        ivm64_rtx_costs(XEXP(x,0),mode,code, 0, &total_0, speed);
        ivm64_rtx_costs(XEXP(x,1),mode,code, 0, &total_1, speed);
        *total = ivm64_op_cost(code)*4 + total_0 + total_1;
        return true;
    }

    return false; // Let gcc recurse
}



/* Output a string based on name, suitable for the ‘#ident’ directive */
void ivm64_asm_output_ident(const char *ident_str)
{
    const char *ident_asm_op = "#\t.ident\t";
    fprintf (asm_out_file, "%s\"%s\"\n", ident_asm_op, ident_str);
}


/* Print an integer */
bool ivm64_asm_integer(rtx x, unsigned int size, int aligned_p ATTRIBUTE_UNUSED)
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
    if (outgoing && global_options.x_optimize > 0 && global_options.x_flag_tree_coalesce_vars){
         ret = gen_rtx_MEM(mode, gen_rtx_PLUS(DImode, frame_pointer_rtx, GEN_INT(0)));
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



/* Initialize the GCC target structure.  */

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

struct gcc_target targetm = TARGET_INITIALIZER;

