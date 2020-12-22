/*
 * Preservation Virtual Machine Project
 * Definitions of target machine for GNU compiler for ivm64 target
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: Oct 2019 - Nov 2020
 *
 */

#ifndef GCC_IVM64_H
#define GCC_IVM64_H

/** RUN-TIME TARGET SPECIFICATIONS **/

#define TARGET_IVM64

/* Target CPU builtins  */
#define TARGET_CPU_CPP_BUILTINS() ivm64_cpu_cpp_builtins(pfile)

/* This macro is used by ivm.c:ivm64_asm_file_start() */
#define IVM64_GCC_VERSION "1.2rc1"
#define VERSION_INFO "GCC Cross Compiler for ivm64, version " IVM64_GCC_VERSION " (" __DATE__ ", " __TIME__ ")"

#ifndef TARGET_DEFAULT
#define TARGET_DEFAULT 0
#endif

/** SOME PER-FUNCTION INITIALIZATION **/

#define ASM_OUTPUT_FUNCTION_PREFIX(FILE, NAME) \
  fprintf(FILE, "\n" ASM_COMMENT_START " FUNCTION BEGINS: %s\n", current_function_name())

/* Macro called to initialize any target specific information.  This macro is
   called onceper function, before generation of any RTL has begun.  The
   intention of this macrois to allow the initialization of the function
   pointerinit_machine_status. */
#define INIT_EXPANDERS ivm64_init_expanders()


/** DRIVER **/

/* Define this macro to be a C string representing the suffix for
   object files on your machine. If you do not define this macro,
   GNU CC will use `.o' as the suffix for object files. */
#define TARGET_OBJECT_SUFFIX ".o"

/* C string constant that tells the GNU CC driver program options to
   pass to the linker. It can also specify how to translate options
   you give to GNU CC into options for GNU CC to pass to the linker.
   `LIB_SPEC' is used at the end of the command given to the linker.
   If this macro is not defined, a default is provided that loads the
   standard C library from the usual place. See `gcc.c'. */
#define LIB_SPEC "-lc"

/* C string constant that tells the GNU CC driver program how
   and when to place a reference to `libgcc.a' into the linker
   command line. This constant is placed both before and after the
   value of `LIB_SPEC'.
   If this macro is not defined, the GNU CC driver provides a default
   that passes the string `-lgcc' to the linker unless the `-shared'
   option is specified. */
#define LIBGCC_SPEC "-lgcc"

/* C string constant that tells the GNU CC driver program how
   and when to place a reference to a startfile, like 'crt0.o',
   into the linker command line. */
#define STARTFILE_SPEC "crt0%O%s"


/** STORAGE LAYOUT **/

/* Define this if most significant bit is lowest numbered
   in instructions that operate on numbered bit-fields. */
#define BITS_BIG_ENDIAN 0

/* Define this if most significant byte of a word is the lowest numbered. */
#define BYTES_BIG_ENDIAN 0

/* Define this if most significant word of a multiword number is the lowest
 numbered. */
#define WORDS_BIG_ENDIAN 0


/* Width in bits of a "word", which is the contents of a machine
   register. If you do not define this macro, the default
   is BITS_PER_UNIT * UNITS_PER_WORD*/
#undef BITS_PER_WORD
#define BITS_PER_WORD 64

#undef MAX_BITS_PER_WORD
#define MAX_BITS_PER_WORD 64

/* Width of a word, in units (bytes). */
#define UNITS_PER_WORD 8
#define MAX_UNITS_PER_WORD 8

/* Width in bits of a pointer.
   See also the macro `Pmode' defined below. */
#define POINTER_SIZE 64

/* Allocation boundary (in *bits*) for storing arguments in argument list. */
#define PARM_BOUNDARY 64

/* Allocation boundary (in *bits*) for the code of a function. */
#define FUNCTION_BOUNDARY 64

/* Alignment of field after `int : 0' in a structure. */
#define EMPTY_FIELD_BOUNDARY 64

/* Every structure's size must be a multiple of this. */
#define STRUCTURE_SIZE_BOUNDARY 64

/* A bitfield declared as `int' forces `int' alignment for the struct. */
#define PCC_BITFIELD_TYPE_MATTERS 1

/* No data type wants to be aligned rounder than this. */
#define BIGGEST_ALIGNMENT 64

/* No structure field wants to be aligned rounder than this. */
#define BIGGEST_FIELD_ALIGNMENT BIGGEST_ALIGNMENT

/* Set this nonzero if move instructions will actually fail to work
   when given unaligned data. */
#define STRICT_ALIGNMENT 1

/* Keep the stack aligned. */
#define STACK_BOUNDARY 64

/* An integer expression for the size in bits of the largest integer
   machine mode that should actually be used. All integer machine
   modes of this size or smaller can be used for structures and
   unions with the appropriate sizes. If this macro is undefined,
  `GET_MODE_BITSIZE (DImode)' is assumed. */
#define MAX_FIXED_MODE_SIZE GET_MODE_BITSIZE (DImode)

/* The best alignment to use in cases where we have a choice.  */
#define FASTEST_ALIGNMENT 64

/* Similarly, make sure that objects on the stack are sensibly aligned. */
#define DATA_ALIGNMENT(EXP, ALIGN) (((ALIGN)<64)?64:(ALIGN))
#define LOCAL_ALIGNMENT(EXP, ALIGN) (((ALIGN)<64)?64:(ALIGN))

#define STACK_SLOT_ALIGNMENT(TYPE,MODE,ALIGN) (((ALIGN)<64)?64:(ALIGN))

/*Biggest alignment supported by the object file format of this machine. */
#define MAX_OFILE_ALIGNMENT (((unsigned int) 1 << 28)*8)


/* Layout of Source Language Data Types */

#define CHAR_TYPE_SIZE  8
#define SHORT_TYPE_SIZE 16
#define INT_TYPE_SIZE   32
#define LONG_TYPE_SIZE  64
#define LONG_LONG_TYPE_SIZE 64

#define FLOAT_TYPE_SIZE 32
#define DOUBLE_TYPE_SIZE 64
#define LONG_DOUBLE_TYPE_SIZE 64

/* Define this as 1 if `char' should by default be signed; else as 0. */
#define DEFAULT_SIGNED_CHAR 1

/* A C expression for a string describing the name
   of the data type to use for size values*/
#define SIZE_TYPE "long unsigned int"

/* C expressions for the standard types.
   This is required by newlib. */
#define SIG_ATOMIC_TYPE "int"
#define INT8_TYPE "char"
#define INT16_TYPE "short int"
#define INT32_TYPE "int"
#define INT64_TYPE "long int"
#define UINT8_TYPE "unsigned char"
#define UINT16_TYPE "short unsigned int"
#define UINT32_TYPE "unsigned int"
#define UINT64_TYPE "long unsigned int"

#define INT_LEAST8_TYPE "char"
#define INT_LEAST16_TYPE "short int"
#define INT_LEAST32_TYPE "int"
#define INT_LEAST64_TYPE "long int"
#define UINT_LEAST8_TYPE "unsigned char"
#define UINT_LEAST16_TYPE "short unsigned int"
#define UINT_LEAST32_TYPE "unsigned int"
#define UINT_LEAST64_TYPE "long unsigned int"

#define INT_FAST8_TYPE "char"
#define INT_FAST16_TYPE "short int"
#define INT_FAST32_TYPE "int"
#define INT_FAST64_TYPE "long int"
#define UINT_FAST8_TYPE "unsigned char"
#define UINT_FAST16_TYPE "short unsigned int"
#define UINT_FAST32_TYPE "unsigned int"
#define UINT_FAST64_TYPE "long unsigned int"

#define INTPTR_TYPE "long int"
#define UINTPTR_TYPE "long unsigned int"


/** REGISTERS **/

/* This is the ivm64 register layout:
     Special purpose registers:
        0  SP - STACK POINTER
        1  FP - FRAME POINTER
        2  TR - Top of stack REGISTER

     General purpose registers:
        3  FIRST_GP_REGNUM = AR
        4  X1
        ...
        18 X15

    Note:
      (AR) used for 64-bit return value (or smaller)
      (X1, AR) used for 128-bit return value
*/

/* Number of actual hardware registers.
   The hardware registers are assigned numbers for the compiler
   from 0 to just below FIRST_PSEUDO_REGISTER.
   All registers that the compiler knows about must be given numbers,
   even those that are not normally considered general registers. */
#define FIRST_PSEUDO_REGISTER (3+16)

/* An initializer that says which registers are used for fixed
   purposes all throughout the compiled code and are therefore not
   available for general allocation.  These would include the stack
   pointer, the frame pointer (except on machines where that can be
   used as a general register when no frame pointer is needed), the
   program counter on machines where that is considered one of the
   addressable registers, and any other numbered register with a
   standard use.

   This information is expressed as a sequence of numbers, separated
   by commas and surrounded by braces.  The Nth number is 1 if
   register N is fixed, 0 otherwise. */
#define FIXED_REGISTERS \
   {1, 1, 1,              /* SP, FP, TR */ \
    0, 0,                 /* AR, X1  */    \
    0, 0, 0, 0, 0, 0, 0,  /* X2..X8  */    \
    0, 0, 0, 0, 0, 0, 0}  /* X9..X15 */

/* Like 'FIXED_REGISTERS' but has 1 for each register that is
   clobbered (in general) by function calls as well as for fixed
   registers.  This macro therefore identifies the registers that are
   not available for general allocation of values that must live
   across function calls.

   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.

   If a register has 0 in `CALL_USED_REGISTERS', the compiler
   automatically saves it on function entry and restores it on
   function exit, if the register is used within the function.  */
#define CALL_USED_REGISTERS \
   {1, 1, 1,              /* SP, FP, TR */ \
    1, 1,                 /* AR, X1  */    \
    0, 0, 0, 0, 0, 0, 0,  /* X2..X8  */    \
    0, 0, 0, 0, 0, 0, 0}  /* X9..X15 */

/* A C initializer containing the assembler's names for the machine
 registers, each one as a C string constant. This is what
 translates register numbers in the compiler into assembler
 language. */
#define REGISTER_NAMES \
  {"SP", "FP", "TR",                                \
   "AR", "X1",                                      \
   "X2", "X3",  "X4",  "X5",  "X6",  "X7",  "X8",   \
   "X9", "X10", "X11", "X12", "X13", "X14", "X15"}

/* Register to use for pushing function arguments. */
#define STACK_POINTER_REGNUM 0

/* Base register for access to local variables of the function. */
#define FRAME_POINTER_REGNUM 1

/* Base register for access to arguments of the function. */
#define ARG_POINTER_REGNUM FRAME_POINTER_REGNUM

/* ivm64 stack mapped general purpose registers*/
#define FIRST_GP_REGNUM  3
#define NUM_GP_REGISTERS (FIRST_PSEUDO_REGISTER - FIRST_GP_REGNUM)

/* AR, X0 are alias of the same register */
#define AR_REGNUM 3
#define X0_REGNUM (AR_REGNUM)
#define X1_REGNUM (X0_REGNUM + 1)

#define IS_GP_REGNUM(REGNO) \
  (((REGNO) >= FIRST_GP_REGNUM) && ((REGNO) < FIRST_PSEUDO_REGISTER))


/** REGISTERS CLASSES **/

/* On many machines, the numbered registers are not all equivalent.
   For example, certain registers may not be allowed for indexed
   addressing; certain registers may not be allowed in some
   instructions.  These machine restrictions are described to the
   compiler using "register classes".

   `enum reg_class'

   An enumeral type that must be defined with all the register class
   names as enumeral values.  `NO_REGS' must be first.  `ALL_REGS'
   must be the last register class, followed by one more enumeral
   value, `LIM_REG_CLASSES', which is not a register class but rather
   tells how many classes there are.

   The classes must be numbered in nondecreasing order; that is,
   a larger-numbered class must never be contained completely
   in a smaller-numbered class.

   One of the classes must be named GENERAL_REGS.  There is nothing
   terribly special about the name, but the operand constraint letters
   ‘r’ and ‘g’ specify this class.  If GENERAL_REGS is the same as
   ALL_REGS, just define it as a macro which expands to ALL_REGS.
   Order the classes so that if class x is contained in class y then
   x has a lower class number than y.

   Each register class has a number, which is the value of casting the
   class name to type `int'.  The number serves as an index in many of
   the tables described below. */

enum reg_class
{
    NO_REGS,
    GP_REG,
    TR_REG,
    GPTR_REGS,
    NOTR_REGS,
    ALL_REGS,
    LIM_REG_CLASSES
};
#define N_REG_CLASSES (int) LIM_REG_CLASSES

/* Notice that GENERAL_REGS corresponds to constraint 'r'*/
#define GENERAL_REGS GPTR_REGS

/* An initializer containing the names of the register classes as C
   string constants.  These names are used in writing some of the
   debugging dumps. */
#define REG_CLASS_NAMES \
 {"NO_REGS", "GP_REG", "TR_REG", "GPTR_REGS", "NOTR_REGS", "ALL_REGS"}

/* 'REG_CLASS_CONTENTS': An initializer containing the contents of the
   register classes, as integers which are bit masks.  The Nth integer
   specifies the contents of class N.  The way the integer MASK is
   interpreted is that register R is in the class if `MASK & (1 << R)' is 1. */
#define HEX_GP_REG    (((1<<NUM_GP_REGISTERS)-1) << 3)
#define HEX_TR_REG    0x4
#define HEX_GPTR_REG  (HEX_GP_REG | HEX_TR_REG)
#define HEX_NOTR_REG  (HEX_GP_REG | 0x3)
#define HEX_ALL_REG   (HEX_GP_REG | 0x7)
#define REG_CLASS_CONTENTS {0x0000, HEX_GP_REG, HEX_TR_REG, HEX_GPTR_REG, HEX_NOTR_REG, HEX_ALL_REG}

/* The same information, inverted:
   A C expression whose value is a register class containing hard
   register REGNO.  In general there is more than one such class;
   choose a class which is "minimal", meaning that no smaller class
   also contains the register. */
#define REGNO_REG_CLASS(REGNO)\
 ((IS_GP_REGNUM(REGNO)) ? GP_REG : ((REGNO) == 2) ? TR_REG : ALL_REGS)


/* A macro whose definition is the name of the class to which a valid index
   register mustbelong.  An index register is one used in an address where its
   value is either multipliedby a scale factor or added to another register (as
   well as added to a displacement). */
#define INDEX_REG_CLASS NOTR_REGS

/* A macro whose definition is the name of the class to which a valid
   index register must belong.  An index register is one used in an
   address where its value is either multiplied by a scale factor or
   added to another register (as well as added to a displacement). */
#define BASE_REG_CLASS  NOTR_REGS

/* Given an rtx X being reloaded into a reg required to be
   in class CLASS, return the class of reg to actually use.
   In general this is just CLASS; but on some machines
   in some cases it is preferable to use a more restrictive class. */
#define PREFERRED_RELOAD_CLASS(X,CLASS) (CLASS)

/* A C expression for the maximum number of consecutive
   registers of class CLASS needed to hold a value of mode MODE. */
#define CLASS_MAX_NREGS(CLASS, MODE)\
 ((GET_MODE_SIZE(MODE) + UNITS_PER_WORD - 1) / UNITS_PER_WORD)

/* These assume that NUM is a hard or pseudo reg number.
   They give nonzero only if NUM is a hard reg of the suitable class
   or a pseudo reg currently allocated to a suitable hard reg.
   Since they use reg_renumber, they are safe only once reg_renumber
   has been allocated, which happens in local-alloc.c. */
#define REGNO_OK_FOR_BASE_P(NUM)\
  ((NUM) == STACK_POINTER_REGNUM ||\
   (NUM) == FRAME_POINTER_REGNUM ||\
   IS_GP_REGNUM(NUM) || IS_GP_REGNUM((unsigned) reg_renumber[NUM]))

/* A C expression which is nonzero if register number NUM is suitable
   for use as an index register in operand addresses.  It may be
   either a suitable hard register or a pseudo register that has been
   allocated such a hard register.

   The difference between an index register and a base register is
   that the index register may be scaled.  If an address involves the
   sum of two registers, neither one of them scaled, then either one
   may be labeled the "base" and the other the "index"; but whichever
   labeling is used must fit the machine's constraints of which
   registers may serve in each capacity.  The compiler will try both
   labelings, looking for one that is valid, and will reload one or
   both registers only if neither labeling works. */
#define REGNO_OK_FOR_INDEX_P(NUM) (IS_GP_REGNUM(NUM))


/** STACK LAYOUT AND CALLING CONVENTION **/

/* Define this if pushing a word onto the stack
   moves the stack pointer a smaller address. */
#define STACK_GROWS_DOWNWARD 1

/* Define this macro to nonzero value if the addresses of local
   variable slots are at negative offsets from the frame pointer. */
#define FRAME_GROWS_DOWNWARD 1

/* Offset from the argument pointer register to the first argument's
   address.  On some machines it may depend on the data type of the
   function.  If 'ARGS_GROW_DOWNWARD', this is the offset to the
   location above the first argument's address.  */
#define FIRST_PARM_OFFSET(FNDECL) (0)

/* If defined, this macro specifies a table of register pairs used to
   eliminate unneeded registers that point into the stack frame.  If
   it is not defined, the only elimination attempted by the compiler
   is to replace references to the frame pointer with references to
   the stack pointer.

   The definition of this macro is a list of structure
   initializations, each of which specifies an original and
   replacement register.

   On some machines, the position of the argument pointer is not known
   until the compilation is completed.  In such a case, a separate
   hard register must be used for the argument pointer.  This register
   can be eliminated by replacing it with either the frame pointer or
   the argument pointer, depending on whether or not the frame pointer
   has been eliminated.

   Note that the elimination of the argument pointer with the stack
   pointer is specified first since that is the preferred elimination.  */
#define ELIMINABLE_REGS  \
    {{ARG_POINTER_REGNUM, STACK_POINTER_REGNUM}, \
     {ARG_POINTER_REGNUM, FRAME_POINTER_REGNUM}, \
     {FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM}}

/* This macro returns the initial difference between the specified pair of
 * registers. The value would be computed from information such as the result
 * of get_frame_size () and the tables of registers df_regs_ever_live_p and
 * call_used_regs.
 */
#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) \
  do{ (OFFSET) = ivm64_initial_elimination_offset((FROM), (TO)); } while (0)

/* This target hook should returntrue if a function must have and use a frame
   pointer. This target hook is called in the reload pass.  If its return value
   is true the functionwill have a frame pointer */
#define TARGET_FRAME_POINTER_REQUIRED hook_bool_void_false

/* A C expression that is the number of bytes actually pushed onto the
  stack when an instruction attempts to push NPUSHED bytes. */
#define PUSH_ROUNDING(BYTES) ivm64_push_rounding(BYTES)

/* A C expression that is nonzero if REGNO is the number of a hard
   register in which function arguments are sometimes passed.  This
   does *not* include implicit arguments such as the static chain and
   the structure-value address.  On many machines, no registers can be
   used for this purpose since all function arguments are pushed on
   the stack. This is the case for ivm64 for which this is always 0. */
#define FUNCTION_ARG_REGNO_P(N) 0

/* A C type for declaring a variable that is used as the first
   argument of `FUNCTION_ARG' and other related values. For some
   target machines, the type `int' suffices and can hold the number
   of bytes of argument so far.
   There is no need to record in `CUMULATIVE_ARGS' anything about the
   arguments that have been passed on the stack. The compiler has
   other variables to keep track of that. For target machines on
   which all arguments are passed on the stack, there is no need to
   store anything in `CUMULATIVE_ARGS'; however, the data structure
   must exist and should not be empty, so use `int'. */
#define CUMULATIVE_ARGS int
#define INIT_CUMULATIVE_ARGS(CUM,FNTYPE,LIBNAME,FNDECL,N_NAMED_ARGS) ((CUM) = 0)

/* A C expression to create an RTX representing the place
   where a  library function returns a value of mode MODE. */
#define LIBCALL_VALUE(MODE) ivm64_libcall_value((MODE))

/** Aggregate Return **/

/* In ivm64 thee caller is made responsible for reserving place
   for the returning aggregate type. The address to the
   reserved memory area is passed in register AR*/
#define DEFAULT_PCC_STRUCT_RETURN 1

/* If the structure value address is not passed in a register, define
   `STRUCT_VALUE' as an expression returning an RTX for the place
   where the address is passed. If it returns 0, the address is
   passed as an "invisible" first argument. */
#define TARGET_STRUCT_VALUE_RTX ivm64_struct_value_rtx

/* Define this macro as a C expression that is nonzero if the return
   instruction or the function epilogue ignores the value of the stack
   pointer; in other words, if it is safe to delete an instruction to
   adjust the stack pointer before a return from the function.

   Note that this macro's value is relevant only for functions for
   which frame pointers are maintained.  It is never safe to delete a
   final stack adjustment in a function that has no frame pointer, and
   the compiler knows this regardless of `EXIT_IGNORE_STACK'.
   No definition is equivalent to always zero.

   Target ivm64 needs always to keep track of the stack pointer */
#define EXIT_IGNORE_STACK 0

/* A C statement or compound statement to output to file
   some assembler code to call the profiling subroutine mcount*/
#define FUNCTION_PROFILER(FILE, LABELNO)

/* Trampolines for Nested Functions

   A trampoline is a small piece of code that is created at run time
   when the address of a nested function is taken. It normally resides
   on the stack, in the stack frame of the containing function. These
   macros tell GCC how to generate code to allocate and initialize a
   trampoline.

   The instructions in the trampoline must do two things: load a
   constant address into the static chain register, and jump to the
   real address of the nested function. On CISC machines such as the
   m68k, this requires two instructions, a move immediate and a
   jump. Then the two addresses exist in the trampoline as word-long
   immediate operands. On RISC machines, it is often necessary to load
   each address into a register in two parts. Then pieces of each
   address form separate immediate operands.

   The code generated to initialize the trampoline must store the
   variable parts--the static chain value and the function
   address--into the immediate operands of the instructions. On a CISC
   machine, this is simply a matter of copying each address to a
   memory reference at the proper offset from the start of the
   trampoline. On a RISC machine, it may be necessary to take out
   pieces of the address and store them separately. */
/* Trampolines are not supported by this target */
#define TARGET_ASM_TRAMPOLINE_TEMPLATE NULL
#define TRAMPOLINE_SIZE 0


/** LIBRARY CALLS **/

/* This hook should declare additional library routines or rename
   existing ones, using the functions set_optab_lib func and
   init_one_libfunc, defined in ‘optabs.c’. init_optabs calls this
   macro after initializing all the normal library routines. The
   default is to do nothing. Most ports don’t need to define this hook. */
#define TARGET_INIT_LIBFUNCS ivm64_init_libfuncs


/** ADDRESSING MODES **/

/* Maximum number of registers that can appear in a valid memory address. */
#define MAX_REGS_PER_ADDRESS 1

/* The macros REG_OK_FOR..._P assume that the arg is a REG rtx
   and check its validity for a certain class.  We have two alternate
   definitions for each of them.  The usual definition accepts all pseudo regs;
   the other rejects them unless they have been allocated suitable hard regs.
   The symbol REG_OK_STRICT causes the latter definition to be used.  Most
   source files want to accept pseudo regs in the hope that they will get
   allocated to the class that the insn wants them to be in.  Source files for
   reload pass need to be strict.  After reload, it makes no difference, since
   pseudo regs have
   been eliminated by then. */
#ifdef REG_OK_STRICT
/* Nonzero if X is a hard reg that can be used as a base reg. */
#define REG_OK_FOR_BASE_P(X) REGNO_OK_FOR_BASE_P(REGNO(X))
#else
/* Nonzero if X is a hard reg that can be used as an base
 or if it is a pseudo reg. */
#define REG_OK_FOR_BASE_P(X) \
 (REGNO_OK_FOR_BASE_P(REGNO(X)) || REGNO(X) >= FIRST_PSEUDO_REGISTER)
#endif
/* Idem for index */
#define REG_OK_FOR_INDEX_P(X) REG_OK_FOR_BASE_P(X)

/*This  hook  returns  true  if x is a legitimate constant for a mode-mode
  immediate operand on the target machine. You can assume that x satisfies
  CONSTANT_P, so you need not check this. The default definition
  returns true. */
#define TARGET_LEGITIMATE_CONSTANT_P ivm64_legitimate_constant_p

/* `TARGET_LEGITIMATE_ADDRESS_P' hook returns whether x (an RTX) is a
   legitimate memory address on the target machine for a memory
   operand of mode mode*/

/* Target ivm64 considers the following groups of legitimate memory
   accesses:

   Simple (Absolute, PC or SP relative):
   (only one load/store)
        mem[constant] (integers/labels)
        mem[constant + constant] (int+int, label+int)
        mem[SP]
        mem[SP + constant]

   One indirection (indirect):
   (one load + one load/store)
        mem[mem[simple_addr]] where simple_addr in
                              {constant, const+const, SP, SP+const}
        mem[reg] (reg may be a GPR or a pseudoregister not being virtual
                  register; these pseudoreg. will be converted to GPRs or
                  stack positions (spilling) after register allocation
                  (reload). As GPRs are stack mapped, mem[GPR] involves
                  one indirection)

   One indirection with offset (indirect relative):
   (one load + add offset + one load/store)
        mem[mem[simple_addr] + offset]
        mem[reg + offset]
        mem[mem[simple_addr]] (offset 0)
        mem[reg] (offset 0)

*/

#define SUM_OF_CONST_P(X)     \
 ((GET_CODE (X) == PLUS)       \
  && CONST_INT_P (XEXP (X, 0)) \
  && CONST_INT_P (XEXP (X, 1)) \
 ) \
 || \
 ((GET_CODE (X) == PLUS)       \
  && (GET_CODE(XEXP (X, 0)) == SYMBOL_REF) \
  && CONST_INT_P (XEXP (X, 1)) \
 )

/* A C expression that is 1 if the RTX X is a constant which is a
 valid address. On most machines, this can be defined as
 `CONSTANT_P (X)', but a few machines are more restrictive in which
 constant addresses are supported. */
#define CONSTANT_ADDRESS_P(X) \
 (GET_CODE (X) == LABEL_REF \
 || GET_CODE (X) == SYMBOL_REF \
 || GET_CODE (X) == CONST \
 || SUM_OF_CONST_P (X))


/* X is a register that will appear as a function
   of SP after register allocation (reload) */
#define STACK_RELATED_REG_P(X) \
    (REG_P(X) \
     && (REGNO(X) == STACK_POINTER_REGNUM \
         || REGNO(X) == FRAME_POINTER_REGNUM \
         || (REGNO(X) >= FIRST_PSEUDO_REGISTER \
             && REGNO(X) <= LAST_VIRTUAL_REGISTER) \
        ) \
    )

/* Stack relative addresses also allows virtual registers
 (virtual_stack_vars_rtx), so it will recognize the stack/frame
 pointer before register allocation. */
#define STACK_RELATIVE_P(X) \
 ( (GET_CODE (X) == PLUS \
     && STACK_RELATED_REG_P(XEXP(X, 0)) \
     && GET_CODE (XEXP (X, 1)) == CONST_INT \
    )\
    || STACK_RELATED_REG_P(X) \
  )

#define PC_OR_STACK_RELATIVE_P(X)\
 (CONSTANT_ADDRESS_P (X) || STACK_RELATIVE_P (X))

/* We must permit GPR relative addresses (a GPR as a base register).
 It's not possible to generate RTL with only one base register
 (the SP - stack pointer).
 As GPR registers reside on the stack. So we must be able to
 handle stack indirect addresses. Symbol_refs is handled in the
 same way as stack references. We can therefore permit all kind
 of MEM (PC or stack relative) indirect addresses and also
 use them in relative/"base register"-addressing.
 A pseudo register is eventually transformed into a GPR.
 register or a stack-position. So, like the GPR registers,
 all pseudo registers are also equivalent to a MEM-indirect
 address. */
#define MEM_INDIRECT_P(X) \
 ( ( GET_CODE (X) == MEM && PC_OR_STACK_RELATIVE_P (XEXP (X, 0))) \
     || (REG_P (X) \
         && ( IS_GP_REGNUM(REGNO (X)) \
              || (REGNO (X) > LAST_VIRTUAL_REGISTER)) \
        ) \
 )

#define MEM_INDIRECT_RELATIVE_P(X)\
 ( (GET_CODE (X) == PLUS \
    && MEM_INDIRECT_P (XEXP (X, 0)) \
    && GET_CODE (XEXP (X, 1)) == CONST_INT \
    && INTVAL (XEXP (X, 1)) >= 0 \
   ) \
   || MEM_INDIRECT_P (X))


#undef TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P ivm64_legitimate_address_p


/** COSTS **/

/* Nonzero if access to memory by bytes is slow and undesirable. */
#define SLOW_BYTE_ACCESS 0

/* Define this macro if it is as good or better to call a constant
   function address than to call an address kept in a register. */
#define NO_FUNCTION_CSE 1


/** SECTIONS **/

#define TEXT_SECTION_ASM_OP "#\t.text"
#define DATA_SECTION_ASM_OP "#\t.data"
#define BSS_SECTION_ASM_OP  "#\t.bss"


/** ASSEMBLER FORMAT - Overall Framework **/

#define ASM_COMMENT_START "#"

#undef  ASM_APP_ON
#define ASM_APP_ON  "#APP\n"

#undef  ASM_APP_OFF
#define ASM_APP_OFF "#NO_APP\n"

/** ASSEMBLER FORMAT - Output of Data **/

/* These hooks specify assembly directives for creating certain kinds of
   integer object. The TARGET_ASM_BYTE_OP directive creates a byte-sized
   object, the TARGET_ASM_ALIGNED_HI_OP one creates an aligned
   two-byte object, and so on. Any of the hooks may be NULL, indicating
   that no suitable directive is available. */
/* Defined as NULL to force to use TARGET_ASM_INTEGER */
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
#define TARGET_ASM_INTEGER ivm64_asm_integer

/* A C statement to output to the stdio stream STREAM an assembler
  instruction to assemble a string constant containing the LEN bytes
  at PTR. */
#define ASM_OUTPUT_ASCII(STREAM, PTR, LEN) \
   do{ ivm64_output_ascii((STREAM), (PTR), (LEN)); }while(0)

/* Define the parentheses used to group arithmetic operations
   in assembler code. */
#define TARGET_ASM_OPEN_PAREN  "("
#define TARGET_ASM_CLOSE_PAREN ")"


/** ASSEMBLER FORMAT - Output of Uninitialized Variables **/

/* Note that this ALIGNMENT is in bits but __attribute__((aligned(N))) is in
   bytes */
#define IVM64_ALIGN(SIZE, ALIGNMENT) \
   (((SIZE)+(ALIGNMENT)/8-1)/((ALIGNMENT)/8)*((ALIGNMENT)/8))

/* This says how to output an assembler line
   to define a global common symbol. */
#define ASM_OUTPUT_COMMON(FILE, NAME, SIZE, ROUNDED)\
    ivm64_initialize_uninitialized_data((FILE), (NAME), (SIZE), (ROUNDED), 1)

#define ASM_OUTPUT_ALIGNED_COMMON(FILE, NAME, SIZE, ALIGNMENT) \
    ASM_OUTPUT_COMMON((FILE), (NAME), (SIZE), IVM64_ALIGN((SIZE), (ALIGNMENT)))

#define ASM_OUTPUT_ALIGNED_DECL_COMMON(FILE, DECL, NAME, SIZE, ALIGNMENT) \
    ASM_OUTPUT_ALIGNED_COMMON((FILE), (NAME), (SIZE), (ALIGNMENT))

/* This says how to output an assembler line
   to define a local common symbol. */
#define ASM_OUTPUT_LOCAL(FILE, NAME, SIZE, ROUNDED)\
    ivm64_initialize_uninitialized_data((FILE), (NAME), (SIZE), (ROUNDED), 0)

#define ASM_OUTPUT_ALIGNED_LOCAL(FILE, NAME, SIZE, ALIGNMENT) \
    ASM_OUTPUT_LOCAL((FILE), (NAME), (SIZE), IVM64_ALIGN((SIZE), (ALIGNMENT)))

#define ASM_OUTPUT_ALIGNED_DECL_LOCAL(FILE, DECL, NAME, SIZE, ALIGNMENT) \
    ASM_OUTPUT_ALIGNED_LOCAL((FILE), (NAME), (SIZE), (ALIGNMENT))


/** ASSEMBLER FORMAT - Output and Generation of Labels **/

/* A C statement (sans semicolon) to output to the stdio stream STREAM
   the assembler definition of a label named NAME. Use the expression
   `assemble_name (STREAM, NAME)' to output the name itself; before
   and after that, output the additional assembler syntax for defining
   the name, and a newline. */
#define ASM_OUTPUT_LABEL(FILE, NAME)\
 do { assemble_name (FILE, NAME); fputs(":\n", FILE); } while (0)

/* Globalizing directive for a label */
#define GLOBAL_ASM_OP "\tEXPORT\t"

/* This target hook is a function to output to a stdio stream some
   commands that will make the label name global; that is, available for
   reference from other files. The default implementation relies on a proper
   definition of `GLOBAL_ASM_OP'. */
#define TARGET_ASM_GLOBALIZE_LABEL ivm64_asm_globalize_label

/* A C statement (sans semicolon) to output to the stdio stream any text
   necessary for declaring the name of an external symbol named NAME which is
   referenced in this compilation but not defined. */
#define ASM_OUTPUT_EXTERNAL(FILE, DECL, NAME)\
 do { fputs("#\tIMPORT ", FILE); assemble_name(FILE, NAME); fputs("\n", FILE);} while (0)

/* A C statement (sans semicolon) to output to the stdio stream FILE a
   reference inassembler syntax to a label named NAME. This should
   add ‘_’ to the front of thename, if that is customary on your
   operating system, as it is in most Berkeley Unix systems. This macro is
   used in `assemble_name'. */
#define ASM_OUTPUT_LABELREF(FILE,NAME) fprintf (FILE, "%s", NAME)

/* Identical to `ASM_OUTPUT_LABEL', except that NAME is known to refer
   to a compiler-generated label*/
#define ASM_OUTPUT_INTERNAL_LABEL(FILE,NAME) \
 do {                                        \
     assemble_name_raw ((FILE), (NAME));     \
     fputs (":\n", (FILE));                  \
 } while (0)

/* A C statement to store into the string LABEL a label whose name is made
   from the string PREFIX and the number NUM. This string, when output
   subsequently by assemble_name, should produce the output that
   (*targetm.asm_out.internal_label) would produce with the same prefix
   and num.
   If the string begins with ‘*’, then `assemble_name' will
   output the rest of the string unchanged. It is often convenient
   for `ASM_GENERATE_INTERNAL_LABEL' to use ‘*’ in this way.
   If the string doesn’t start with ‘*’, then `ASM_OUTPUT_LABELREF'
   gets to output the string, and may change it. (Of course,
   `ASM_OUTPUT_LABELREF' is also part of your machine description,
   so you should know what it does on your machine.) */
#define ASM_GENERATE_INTERNAL_LABEL(LABEL,PREFIX,NUM) \
 sprintf (LABEL, "*.%s%ld", PREFIX, (long)NUM)

/* A C expression to assign to OUTPUT (which is a variable of
   type char *) a newly allocated string made from the string NAME and
   the number LABELNO, with some suitable punctuation added. Use alloca()
   to get space for the string. */
/* Used for static labels */
#define ASM_FORMAT_PRIVATE_NAME(OUTPUT, NAME, LABELNO) \
 ((OUTPUT) = (char *) alloca (strlen ((NAME)) + 10),   \
   sprintf((OUTPUT), ".Li_%s_%d", (NAME), (LABELNO)))

#define NO_DOLLAR_IN_LABEL

/* A C statement to output to the stdio stream FILE assembler code which
   defines (equates) the symbol whose tree node is DECL to have the value
   of the TARGET. This macro will be used in preference to
   ‘ASM_OUTPUT_DEF’ if itis defined and if the tree nodes are available.
   If SET_ASM_OP is defined, a default definition is provided which is
   correct for most systemq */
/* This implements the "alias" attribute. */
#undef ASM_OUTPUT_DEF_FROM_DECLS
#define ASM_OUTPUT_DEF_FROM_DECLS(FILE, DECL, TARGET)        \
  do                                                           \
    {                                                          \
      const char *alias = XSTR (XEXP (DECL_RTL(DECL), 0), 0); \
      const char *xalias = targetm.strip_name_encoding(alias); \
      const char *name = IDENTIFIER_POINTER(TARGET);          \
      const char *xname = targetm.strip_name_encoding(name);   \
      ASM_OUTPUT_DEF ((FILE), xalias, xname);                  \
    }                                                          \
   while (0)

#undef ASM_OUTPUT_DEF
#define ASM_OUTPUT_DEF(FILE, alias, name)   \
  do                                          \
    {                                         \
      fprintf((FILE), "\t%s=%s\t#alias\n",    \
                    (alias), (name));         \
    }                                         \
   while (0)

/* This implements the "weak alias" attribute. */
#define ASM_OUTPUT_WEAKREF(FILE, DECL, NAME, VALUE)         \
  do                                                        \
    {                                                       \
      fprintf((FILE), "\t%s=%s #weak ref\n",                \
                       targetm.strip_name_encoding(NAME),   \
                       targetm.strip_name_encoding(VALUE)); \
    }                                                       \
  while(0)


/** ASSEMBLER FORMAT - Macros Controlling Initialization Routines **/

#define HAS_INIT_SECTION

#define TARGET_HAVE_NAMED_SECTIONS false


/** ASSEMBLER FORMAT - Output of Assembler Instructions **/

/* A C expression which evaluates to true if code is a valid punctuation
   character for use in the `PRINT_OPERAND' macro. If
   `PRINT_OPERAND_PUNCT_VALID_P' is not defined, it means that no punctuation
   characters (except for the standard one, ‘%’) are used in this way. */
#define PRINT_OPERAND_PUNCT_VALID_P(CHAR) ((CHAR) == '#')

/* A C compound statement to output to stdio stream FILE the
   assembler syntax for an instruction operand X. X is an RTL
   expression. */
#define PRINT_OPERAND(FILE, X, CODE) \
  do { print_operand((FILE), (X), (CODE)); } while(0)

/* A C compound statement to output to stdio stream FILE the assembler
   syntax for an instruction operand that is a memory reference whose
   address is X. */
#define PRINT_OPERAND_ADDRESS(FILE, X) \
 ivm64_fatal ("PRINT_OPERAND_ADDRESS: %%a directive not supported.");


/** ASSEMBLER FORMAT - Output of Dispatch Tables **/

/* An alias for a machine mode name. This is the machine mode that
   elements of a jump-table should have. */
#define CASE_VECTOR_MODE SImode

/* Define this macro to be a C expression to indicate when jump-tables should
   contain relative addresses. You need not define this macro if jump-tables
   never contain relative addresses,  or  jump-tables  should  contain
   relative  addresses  only  when  ‘-fPIC’  or‘-fPIC’ is in effect. */
#define CASE_VECTOR_PC_RELATIVE flag_pic


/* How to output an element of a case-vector that is absolute. */
#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE) \
 fprintf (FILE, "\tdata8 [.L%d]\n", VALUE)

/* How to output an element of a case-vector that is relative. */
#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, BODY, VALUE, REL) \
 fprintf (FILE, "\tdata8 [(+ .L%d -.L%d)]\n", VALUE, REL)

/* Define this if the label before a jump-table needs to be output
   specially. */
#define ASM_OUTPUT_CASE_LABEL(STREAM, PREFIX, NUM, TABLE)\
do { \
     char *buf = (char *)alloca(strlen(PREFIX)+16); \
     ASM_GENERATE_INTERNAL_LABEL (buf, PREFIX, NUM); \
     ASM_OUTPUT_INTERNAL_LABEL (STREAM, buf); \
} while(0)


/** ASSEMBLER FORMAT - Assembler Commands for Alignment **/

/* A C statement to output to the stdio stream FILE an assembler command to
   advance the location counter to a multiple of 2 to the LOG bytes. LOG will
   be a C expressionof type int. */
/* No alignment directive is available for ivm64 */
#define ASM_OUTPUT_ALIGN(FILE, LOG) do { } while(0)

/* A C statement to output to the stdio stream FILE an assembler
   instruction to advance the location counter SIZE bytes. Those
   bytes should be zero when loaded. SIZE will be a C expression of
   type unsigned HOST_WIDE_INT */
#define ASM_OUTPUT_SKIP(FILE, SIZE) ivm64_output_data_zero_vector(FILE,SIZE)


/** MISCELLANEOUS **/

/* Define this macro to 1 if operations with different modes occurs in a
   whole register */
#define WORD_REGISTER_OPERATIONS 1

/* Max number of bytes we can move from memory to memory
   in one reasonably fast instruction. */
#define MOVE_MAX UNITS_PER_WORD

/* Value is 1 if truncating an integer of inprec bits to outprec bits
   is done just by pretending it is already truncated. The default returns
   true unconditionally, which is correct for most machines.*/
#define TARGET_TRULY_NOOP_TRUNCATION ivm64_truly_noop_truncation

/* An alias for the machine mode for pointers. On most machines,
   define this to be the integer mode corresponding to the width of a
   hardware pointer; After generation of rtl, the compiler makes no further
   distinction between pointers and any other objects of this machine mode */
#define Pmode DImode

/* An alias for the machine mode used for memory references to
   functions being called. */
#define FUNCTION_MODE DImode

/* A C expression whose value is RTL representing the address of the initial
   stack frame. This address is passed to RETURN_ADDR_RTX and
   DYNAMIC_CHAIN_ADDRESS. If you don’t define this macro, a reasonable default
   value will be used. */
/* This is a fake value as no FP is avaliable in ivm64 */
#define INITIAL_FRAME_ADDRESS_RTX (plus_constant (Pmode, arg_pointer_rtx, -2*UNITS_PER_WORD))

#define STACK_CHECK_BUILTIN 1
#define STACK_CHECK_STATIC_BUILTIN 1


/** SOME SPECIFIC DEFINITIONS FOR THIS TARGET **/


#define IVM64_DISABLE_X_FLAG_TREE_TER 1
#define IVM64_DISABLE_X_FLAG_DELAYED_BRANCH 1
#define IVM64_DISABLE_X_FLAG_DSE            1
#define IVM64_DISABLE_X_FLAG_CROSSJUMPING   1
#define IVM64_DISABLE_X_FLAG_CPROP_REGISTERS 0
#define IVM64_DISABLE_X_FLAG_DELETE_NULL_POINTER_CHECKS 1
#define IVM64_DISABLE_X_FLAG_CSE_FOLLOW_JUMPS 1
#define IVM64_DISABLE_X_FLAG_CSE_SKIP_BLOCKS  0
#define IVM64_DISABLE_X_FLAG_CSE_AFTER_LOOP   1
#define IVM64_DISABLE_X_FLAG_IVOPTS   0
#define IVM64_DISABLE_X_FLAG_TREE_LOOP_OPTIMIZE   0
#define IVM64_DISABLE_X_FLAG_ISOLATE_ERRONEOUS_PATHS_DEREFERENCE 1
#define IVM64_DISABLE_X_FLAG_TREE_CH  0
#define IVM64_DISABLE_X_FLAG_TREE_DCE 0
#define IVM64_DISABLE_X_FLAG_CODE_HOISTING 0
#define IVM64_DISABLE_X_FLAG_TREE_PRE      0
#define IVM64_DISABLE_X_FLAG_EARLY_INLINING         0
#define IVM64_DISABLE_X_FLAG_INLINE_SMALL_FUNCTIONS 0
#define IVM64_DISABLE_X_FLAG_INLINE                 0
#define IVM64_DISABLE_INLINE  0
#define IVM64_DISABLE_X_FLAG_TREE_SRA      1
#define IVM64_DISABLE_X_FLAG_DCE 1
#define IVM64_DISABLE_X_FLAG_TREE_FORWPROP 0
#define IVM64_DISABLE_X_FLAG_EXPENSIVE_OPTIMIZATIONS 0
#define IVM64_DISABLE_X_FLAG_TREE_VECTORIZE 1
#define IVM64_DISABLE_X_FLAG_FAST_MATH  1
#define IVM64_DISABLE_RTL_CSE1 0
#define IVM64_DISABLE_RTL_CSE2 1
#define IVM64_DISABLE_RTL_CPROP 1
#define IVM64_REORDER_BLOCKS_ALGORITHM_STC 1

#define IVM64_OMIT_DEBUG_MESSAGE 1
#define IVM64_OMIT_FP_MESSAGE 1
#define IVM64_OMIT_DEFER_POP_MESSAGE 1

#define IVM64_NON_BUILTIN_ALLOCA 1


/* Peephole control */
enum ivm64_peephole2 {
 IVM64_PEEP2_PUSH_POP                       ,
 IVM64_PEEP2_PUSH_POP_PUSH_POP              ,
 IVM64_PEEP2_MOVE_MOVE                      ,
 IVM64_PEEP2_POP_PUSH_POP_PUSH              ,
 IVM64_PEEP2_POP_MOVE                       ,
 IVM64_PEEP2_POP_MOVE2                      ,
 IVM64_PEEP2_POP_PUSHARG                    ,
 IVM64_PEEP2_MOVE_PUSHARG                   ,
 IVM64_PEEP2_MOVE_BINOP                     ,
 IVM64_PEEP2_BINOP1                         ,
 IVM64_PEEP2_BINOP_1_2_3                    ,
 IVM64_PEEP2_BINOP_1_2                      ,
 IVM64_PEEP2_BINOP_1_DEADREG_BINOP_2        ,
 IVM64_PEEP2_BINOP_1_2_DEADREG_BINOP_3      ,
 IVM64_PEEP2_ZERO_EXTEND                    ,
 IVM64_PEEP2_PUSH_POP_DEADREG_BINOP         ,
 IVM64_PEEP2_UNARY_DEADREG_BINOP            ,
 IVM64_PEEP2_UNARY_1_BINOP_2_DEADREG_BINOP_3,
 IVM64_PEEP2_RDX_1_2                        ,
 IVM64_PEEP2_POP_BLOCK_PUSH_BLOCK           ,
 IVM64_PEEP2_SIGNX                          ,
 IVM64_PEEP2_MOVE_PUSH_COMMUTATIVE          ,
 IVM64_PEEP2_MOVE_PUSH_IMM_COMMUTATIVE      ,
 IVM64_PEEP2_POP_PUSH_SUB                   ,
 IVM64_PEEP2_MOVE_PUSH_SUB                  ,
 IVM64_PEEP2_POP_PUSH_COMMUTATIVE           ,
 IVM64_PEEP2_POP_BLOCK_PUSH_COMMUTATIVE     ,
 IVM64_PEEP2_MOVE_PUSH                      ,
 IVM64_PEEP2_POW2                           ,
 IVM64_PEEP2_MOVE                           ,
 IVM64_PEEP2_POP_PUSH                       ,
 IVM64_PEEP2_POP_PUSHDI                     ,
 IVM64_PEEP2_POPDI_PUSH                     ,
 IVM64_PEEP2_POP_IND_PUSH                   ,
 IVM64_PEEP2_POP_IND_OFFSET_PUSH            ,
 IVM64_PEEP2_POP_CBRANCH                    ,
 IVM64_PEEP2_POP_CBRANCH_REV                ,
 IVM64_PEEP2_POPDI_BLOCK_CBRANCH            ,
 IVM64_PEEP2_POPDI_CBRANCH                  ,
 IVM64_PEEP2_POPDI_CBRANCH_REV              ,
 IVM64_PEEP2_SET_CBRANCH                    ,
 IVM64_PEEP2_SET_CBRANCH_REV                ,
 IVM64_PEEP2_SIGNX_POP_CBRANCH_REV          ,
 IVM64_PEEP2_BLOCK_BLOCK                    ,
 IVM64_PEEP2_SET_NOP                        ,
 IVM64_PEEP2_NOP_SET                        ,
 IVM64_PEEP2_POP_NOP_PUSH                   ,
 IVM64_PEEP2_MOV_PUSH                       ,
 IVM64_PEEP2_MOV_PUSH_POP_PUSH_POP          ,
 IVM64_PEEP2_CALL_PUSH_AR                   ,
 IVM64_PEEP2_BLOCK_POP_BLOCK                ,
 IVM64_PEEP2_PUSH_BINOP                     ,
 IVM64_PEEP2_PUSH_POP_PUSH_BINOP_MULTIMODE  ,
 IVM64_PEEP2_PUSH_INDPUSH_ZERO_EXTEND       ,
 IVM64_PEEP2_PUSH_SUB_NEG                   ,
 IVM64_PEEP2_AND_POP                        ,
 IVM64_PEEP2_PUSH_ADD_POP_PUSH_IND_POP      ,
 IVM64_PEEP2_PUSH_ADD_POP_PUSH_IND_OFFSET_POP,
 IVM64_PEEP2_MOVE_MOVE_MOVE                 ,
 IVM64_PEEP1_MOVE_PUSH_POP                  ,
 IVM64_PEEP1_SHIFTRU63_AND                  ,
 IVM64_PEEP1_PUSHAR_BINOP_POPAR             ,
 IVM64_PEEP1_PUSH_COMMUTATIVE_POPAR         ,
 IVM64_PEEP1_PUSH_SUB_POPAR                 ,
 IVM64_PEEP1_PUSH_BINOP_BINOP_POPAR         ,
 IVM64_PEEP1_PUSH_ADD                       ,
 IVM64_PEEP1_PUSHAR_UNARY_POPAR             ,
 IVM64_PEEP1_PUSHAR_SIGNX_POPAR             ,
 IVM64_PEEP1_POP_PUSH                       ,
 IVM64_PEEP1_PUSH_SIGNX                     ,
 IVM64_PEEP1_IND_PUSH_SIGNX
};


#endif /* GCC_IVM64_H */
