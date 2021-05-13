/*
 * Preservation Virtual Machine Project
 * Prototypes for exported functions defined for the ivm64 target
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: Oct 2019
 *
 */

#ifndef GCC_IVM64_PROTOS_H
#define GCC_IVM64_PROTOS_H

/* Global variables*/

extern int ivm64_gpr_offset;
extern int ivm64_stack_extra_offset;
extern int emitted_push_cfun;
extern int emitted_pop_cfun;
extern int ivm64_prolog_fast_pop;

/* Helper functions*/

extern void ivm64_cpu_cpp_builtins(struct cpp_reader *pfile);
extern void ivm64_fatal (const char *msg);
extern int ivm64_peep_enabled(enum ivm64_peephole2 id);
extern const char *ivm64_rtxop2insn(rtx op);
extern const char *ivm64_rtxcode2insn(int code);
extern int ivm64_pass_in_progress(const char *name);
extern int ivm64_postreload_in_progress();
extern int ivm64_extra_peep2_in_progress(int extra);
extern rtx ivm64_copy_rtx(rtx orig);


/* Expand functions */

extern void ivm64_init_expanders();
extern void ivm64_expand_push(rtx operand, enum machine_mode mode);
extern void ivm64_expand_pop(rtx operand, enum machine_mode mode);
extern int ivm64_expand_move(rtx *operands, enum machine_mode mode);
extern int ivm64_expand_add (rtx *operands, enum machine_mode mode);
#ifdef RTX_CODE
extern int ivm64_expand_commutative(rtx *operands, enum machine_mode mode,
                            enum rtx_code code);
extern int ivm64_expand_non_commutative(rtx *operands, enum machine_mode mode,
                            enum rtx_code code);
extern int ivm64_expand_unary(rtx *operands, enum machine_mode srcmode,
                            enum machine_mode dstmode, enum rtx_code code);
#endif
extern void ivm64_expand_call(rtx *operands);
extern void ivm64_expand_call_value(rtx *operands);
extern void ivm64_expand_call_pop(rtx *operands);
extern void ivm64_expand_call_value_pop(rtx *operands);
extern void ivm64_expand_builtin_longjump_call(rtx *operands);
extern void ivm64_expand_builtin_longjump_inline(rtx *operands);
extern void ivm64_expand_call_alloca(rtx *operands);
extern void ivm64_expand_save_stack_block(rtx *operands);
extern void ivm64_expand_restore_stack_block(rtx *operands);

/* Output functions */

extern void ivm64_output_ascii (FILE *file, const char *ptr, int len);
extern void print_operand (FILE *file, rtx x, int code);
extern void ivm64_output_push(rtx operands, enum machine_mode mode);
extern void ivm64_output_pop(rtx operands, enum machine_mode mode);
extern void ivm64_output_move(rtx *operands, enum machine_mode mode);
extern void ivm64_output_zero_extend(int nbytes);
extern void ivm64_output_sign_extend(int nbytes);
extern void ivm64_output_call(rtx address);
extern long ivm64_output_return_value(rtx *operands, int restore_slots);
extern void ivm64_output_cbranch(rtx *operands, machine_mode mode);
extern void ivm64_output_cbranch_peep(rtx *operands, machine_mode mode, int reverse, rtx_insn *insn);
extern int  ivm64_peep_pop_cmp_p(rtx op, int reverse);
extern void ivm64_output_casesi(rtx *operands);
extern void ivm64_output_setsp(FILE *file, long n);

/* Public functions used in other files (ivm64.h, ivm64.md, ...)*/

extern int is_a_push_memcpy(rtx nbytes);
extern int is_noreturn(rtx call_insn);
extern int return_value_unused(rtx call_insn);
HOST_WIDE_INT ivm64_initial_elimination_offset (int from, int to);
extern void ivm64_initialize_uninitialized_data(FILE *file, const char *name, unsigned long size, int rounded, bool global);
extern void ivm64_output_data_zero_vector(FILE *file, unsigned long size);
poly_int64 ivm64_push_rounding(poly_int64 bytes);
extern rtx ivm64_libcall_value(machine_mode mode);
extern rtx ivm64_return_addr(int count, rtx frame);

#endif /* ! GCC_IVM64_PROTOS_H */
