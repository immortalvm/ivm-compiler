/*
 * Preservation Virtual Machine Project
 *  Machine description of target ivm64 for GNU compiler
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: Oct 2019 - May 2021
 *
 */

;;----------------------------------------------------------------------------
;; Predicates, constraints and attributes
;;----------------------------------------------------------------------------

(include "predicates.md")

(include "constraints.md")

(define_constants
  [
   (SP_REGNUM        0)
   (FP_REGNUM        1)
   (TR_REGNUM        2)
   (AR_REGNUM        3)
])

(define_constants
  [
   (IVM64_ENABLE_MULTIMODE_ARITH 0)
])

(define_c_enum "unspec"[
  UNSPEC_PUSH_TR
  UNSPEC_POP_TR
  ;;
  UNSPEC_PUSH_TR_TR
  UNSPEC_POP_TR_TR
  ;;
  UNSPEC_POP_SP
  ;;
  UNSPEC_BLOCKAGE
  ;;
  ;;-- used in peepholes
  UNSPEC_IND_PUSH_TR
  UNSPEC_IND_OFFSET_PUSH_TR
  UNSPEC_IND_POP_TR
  UNSPEC_POP_PUSHARG_TR
  UNSPEC_POW_TR
  UNSPEC_NEG_TR
  UNSPEC_XOR_TR
  UNSPEC_POP_TR_NOT
  ;;
  UNSPEC_NOP_PUSH_TR
  UNSPEC_NOP_POP_TR
  UNSPEC_PRINT
  UNSPEC_AR_OFFSET
  UNSPEC_EXTRA_OFFSET
  ;;
  UNSPEC_NONLOCAL_GOTO
  ]
)

;;----------------------------------------------------------------------------
;; Iterators
;;----------------------------------------------------------------------------

(define_mode_iterator QIHISIDI [QI HI SI DI])
(define_mode_iterator QIHISI [QI HI SI])
(define_mode_iterator ALLMODES [QI HI SI DI SF DF])
(define_mode_attr MODESIZE [(QI "1") (HI "2") (SI "4") (DI "8")
                            (SF "4") (DF "8")])

;;-------------------------------------------------------------------------
;; Data movement instructions
;;-------------------------------------------------------------------------

(define_insn "push<mode>1_internal"
    [(set (mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
        (match_operand:ALLMODES 0 "general_not_fp_operand" "i,St,m") )]
    ""
     {
        ivm64_output_push(operands[0], <MODE>mode);
        ivm64_gpr_offset += UNITS_PER_WORD;
        return "";
     }
)

(define_expand "push<mode>1"
    [(set (mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
          (match_operand:ALLMODES 0 "" ""))]
    ""
    {
     emit_insn(gen_push<mode>1_internal(operands[0]));
     DONE;
    }
)

(define_expand "mov<mode>"
     [(set (match_operand:ALLMODES 0 "general_operand" "")
      (match_operand:ALLMODES 1 "general_operand" ""))]
     ""
     {
     if (!pushable_operand(operands[1], <MODE>mode)
          || !popable_operand(operands[0], <MODE>mode))
     {
        if (ivm64_expand_move (operands, <MODE>mode))
            DONE;
     }
     extern rtx ivm64_last_set_operand;
     ivm64_last_set_operand = operands[0];
     }
)

(define_insn "*mov<mode>_to_tr"
      [(set (match_operand:ALLMODES 0 "tr_register_operand" "=r,r,r")
            (match_operand:ALLMODES 1 "general_operand" "i,St,m"))]
    "(!ivm64_postreload_in_progress()
      && !tr_register_operand(operands[1], DImode))"
    {
        if ((GET_CODE(operands[1]) == CONST_INT) && (0 == INTVAL(operands[1]))){
             /* TR <- 0 */
             return "and! 0";
        } else if (pushable_operand(operands[1], DImode)) {
             /* TR <- cte, TR <- reg, ... */
             ivm64_output_setsp(asm_out_file, 1);
             ivm64_stack_extra_offset += - UNITS_PER_WORD;
             ivm64_output_push(operands[1], <MODE>mode);
             ivm64_stack_extra_offset += + UNITS_PER_WORD;
             return "";
        }
        ivm64_fatal("Invalid transfer to TR");
        return "";
    }
 )

(define_insn "*mov<mode>_push_pop_tr"
    [(set (reg:QIHISIDI TR_REGNUM)
          (unspec_volatile:QIHISIDI [
                (reg:QIHISIDI TR_REGNUM)
                (unspec_volatile:QIHISIDI [ (reg:QIHISIDI TR_REGNUM) ] UNSPEC_POP_TR)
            ] UNSPEC_PUSH_TR))
    ]
    ""
    {
        ivm64_output_zero_extend(<MODESIZE>);
        return "";
    }
)

(define_insn "mov<mode>_to_tr_unspec"
    [ ( set (reg:ALLMODES TR_REGNUM)
            (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                         (match_operand:ALLMODES 0 "general_operand" "i,St,m")]
             UNSPEC_PUSH_TR)
    )]
    ""
    {
        ivm64_output_push(operands[0], <MODE>mode);
        emitted_push_cfun++;
        return "";
    }
)

(define_insn "mov<mode>_from_tr_unspec"
    [( set (match_operand:ALLMODES 0 "general_dst_operand" "=St,m")
            (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )]
    ""
    {
        ivm64_output_pop(operands[0], <MODE>mode);
        emitted_pop_cfun++;
        return "";
    }
)

(define_insn "mov_from_tr_to_tr_push_unspec"
    ;;[(unspec_volatile [(reg:DI TR_REGNUM)] UNSPEC_PUSH_TR_TR)]
    [(set (reg:DI TR_REGNUM)
          (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_PUSH_TR_TR)
     )
    ]
    ""
    {
        output_asm_insn("push! $0", NULL);
        emitted_push_cfun++;
        return "";
    }
)

(define_insn "mov_from_tr_to_tr_pop_unspec"
    [(set (reg:DI TR_REGNUM)
          (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR_TR)
     )
    ]
    ""
    {
        output_asm_insn("store8! &1", NULL);
        emitted_pop_cfun++;
        return "";
    }
)

(define_insn "mov<mode>_push_pop"
    [ ( set (match_operand:ALLMODES 0 "general_dst_not_tr_sp_operand" "=Qtm,Qtm,Qtm")
            (match_operand:ALLMODES 1 "general_not_tr_operand" "i,St,m")
      )
    ]
    ""
    {
      ivm64_output_move(operands, <MODE>mode);
      return "";
    }
)


;;-------------------------------------------------------------------------
;; Arithmetic instructions
;;-------------------------------------------------------------------------

;; Add --------------------------------------------------------------------

(define_insn "*adddi_sp"
    [(set (match_operand:DI 0 "sp_register_operand" "")
     (plus:DI (match_operand:DI 1 "sp_register_operand" "")
      (match_operand:DI 2 "immediate_operand" "i")))]
    ""
    {
     if (INTVAL(operands[2]) != 0) {
         ivm64_gpr_offset -= INTVAL (operands[2]);
         return "set_sp! (+ &0 %2)";
     } else {
         return "";
     }
    }
)

(define_expand "adddi3"
    [(set (match_operand:DI 0 "general_operand" "")
     (plus:DI (match_operand:DI 1 "general_operand" "")
      (match_operand:DI 2 "general_operand" "")))]
    ""
    {
     if (ivm64_expand_add(operands, DImode))
         DONE;
    }
)

(define_expand "add<mode>3"
    [(set (match_operand:QIHISI 0 "general_operand" "")
     (plus:QIHISI (match_operand:QIHISI  1 "general_operand" "")
      (match_operand:QIHISI 2 "general_operand" "")))]
    "IVM64_ENABLE_MULTIMODE_ARITH"
    {
     if (ivm64_expand_add (operands, <MODE>mode))
         DONE;
    }
)

(define_insn "*add<mode>"
    [(set (match_operand:QIHISIDI 0 "tr_register_operand" "=r,r")
     (plus:QIHISIDI (match_operand:QIHISIDI 1 "tr_register_operand" "r,r")
      (match_operand:QIHISIDI 2 "arithmetic_operand" "i,rm")))]
    ""
    "@
    add! %2
    load<MODESIZE>! %2\;add"
    )

;; Commutative instructions except addition -------------------------------

(define_code_iterator commut_op [mult and ior xor])
(define_code_attr commut_name  [(mult "mul")
                                (and "and") (ior "ior") (xor "xor")])
(define_code_attr commut_code  [(mult "MULT")
                                (and "AND") (ior "IOR") (xor "XOR")])
(define_code_attr commut_insn  [(mult "mult")
                                (and "and") (ior "or")  (xor "xor")])

(define_expand "<commut_name>di3"
    [(set (match_operand:DI 0 "general_operand" "")
     (commut_op:DI (match_operand:DI 1 "general_operand" "")
      (match_operand:DI 2 "general_operand" "")))]
    ""
    {
     if (ivm64_expand_commutative (operands, DImode, <commut_code>))
        DONE;
    }
)

(define_expand "<commut_name><mode>3"
    [(set (match_operand:QIHISI 0 "general_operand" "")
     (commut_op:QIHISI (match_operand:QIHISI 1 "general_operand" "")
      (match_operand:QIHISI 2 "general_operand" "")))]
    "IVM64_ENABLE_MULTIMODE_ARITH"
    {
     if (ivm64_expand_commutative (operands, <MODE>mode, <commut_code>))
         DONE;
    }
)

(define_insn "*<commut_name><mode>3"
    [(set (match_operand:QIHISIDI 0 "tr_register_operand" "=r,r")
     (commut_op:QIHISIDI (match_operand:QIHISIDI 1 "tr_register_operand" "r,r")
      (match_operand:QIHISIDI 2 "arithmetic_operand" "i,rm")))]
    ""
    "@
    <commut_insn>! %2
    load<MODESIZE>! %2\;<commut_insn>"
    )

;; Non-commutative instructions -------------------------------------------

(define_code_iterator noncommut_op [minus div mod udiv umod])
(define_code_attr noncommut_name  [(minus "sub")
                                   (div "div") (mod "mod")
                                   (udiv "udiv") (umod "umod")])
(define_code_attr noncommut_code  [(minus "MINUS")
                                   (div "DIV") (mod "MOD")
                                   (udiv "UDIV") (umod "UMOD")])
(define_code_attr noncommut_insn  [(minus "sub")
                                   (div "div_s") (mod "rem_s")
                                   (udiv "div_u") (umod "rem_u")])

(define_expand "<noncommut_name>di3"
    [(set (match_operand:DI 0 "general_operand" "")
     (noncommut_op:DI (match_operand:DI 1 "general_operand" "")
     (match_operand:DI 2 "general_operand" "")))]
    ""
    {
     if (ivm64_expand_non_commutative(operands, DImode, <noncommut_code>))
     DONE;
    }
)

(define_expand "<noncommut_name><mode>3"
    [(set (match_operand:QIHISI 0 "general_operand" "")
     (noncommut_op:QIHISI (match_operand:QIHISI 1 "general_operand" "")
     (match_operand:QIHISI 2 "general_operand" "")))]
    "IVM64_ENABLE_MULTIMODE_ARITH"
    {
     if (ivm64_expand_non_commutative(operands, <MODE>mode, <noncommut_code>))
         DONE;
    }
)

(define_insn "*<noncommut_name><mode>3"
    [(set (match_operand:QIHISIDI 0 "tr_register_operand" "=r,r")
     (noncommut_op:QIHISIDI (match_operand:QIHISIDI 1 "tr_register_operand" "r,r")
      (match_operand:QIHISIDI 2 "arithmetic_operand" "i,rm")))]
    ""
    "@
    <noncommut_insn>! %2
    load<MODESIZE>! %2\;<noncommut_insn>"
    )


;; Shifters ---------------------------------------------------------------

(define_code_iterator shifter_op [ashiftrt ashift lshiftrt])
(define_code_attr shifter_name  [(ashiftrt "ashr") (ashift "ashl")
                                 (lshiftrt "lshr")])
(define_code_attr shifter_code  [(ashiftrt "ASHIFTRT") (ashift "ASHIFT")
                                 (lshiftrt "LSHIFTRT")])
(define_code_attr shifter_insn  [(ashiftrt "shift_rs") (ashift "shift_l")
                                 (lshiftrt "shift_ru")])

(define_expand "<shifter_name>di3"
    [(set (match_operand:DI 0 "general_operand" "")
     (shifter_op:DI (match_operand:DI 1 "general_operand" "")
     (match_operand:DI 2 "general_operand" "")))]
    ""
    {
     if (ivm64_expand_non_commutative(operands, DImode, <shifter_code>))
     DONE;
    }
)

(define_insn "*<shifter_name>di3"
    [(set (match_operand:DI 0 "tr_register_operand" "=r,r")
     (shifter_op:DI (match_operand:QIHISIDI 1 "tr_register_operand" "r,r")
      (match_operand:DI 2 "arithmetic_operand" "i,rm")))]
    ""
    "@
    <shifter_insn>! %2
    load<MODESIZE>! %2\;<shifter_insn>"
    )


;; Unary operators --------------------------------------------------------

(define_expand "negdi2"
    [(set (match_operand:DI 0 "general_operand" "")
     (neg:DI (match_operand:DI 1 "general_operand" "")))]
    ""
    {
     if (ivm64_expand_unary(operands, DImode, DImode, NEG))
        DONE;
    }
)

(define_expand "one_cmpldi2"
    [(set (match_operand:DI 0 "general_operand" "")
     (not:DI (match_operand:DI 1 "general_operand" "")))]
    ""
    {
     if (ivm64_expand_unary(operands, DImode, DImode, NOT))
       DONE;
    }
)

(define_insn "*negdi2"
    [(set (match_operand:DI 0 "tr_register_operand" "=r")
     (neg:DI (match_operand:DI 1 "tr_register_operand" "r")))]
    ""
    "neg"
    )

(define_insn "*one_cmpldi2"
    [(set (match_operand:DI 0 "tr_register_operand" "=r")
     (not:DI (match_operand:DI 1 "tr_register_operand" "r")))]
    ""
    "not"
    )


;; Extenders --------------------------------------------------------------

(define_expand "zero_extend<mode>di2"
    [(set (match_operand:DI 0 "register_operand" "")
    (zero_extend:DI (match_operand:QIHISI 1 "nonimmediate_operand" "")))]
    ""
    {
     if (ivm64_expand_unary(operands, <MODE>mode, DImode, ZERO_EXTEND))
        DONE;
    }
)

(define_insn "*zero_extend<mode>di2"
    [(set (match_operand:DI 0 "tr_register_operand" "=r")
     (zero_extend:DI (match_operand:ALLMODES 1 "tr_register_operand" "r")))]
    ""
    {
        ivm64_output_zero_extend(<MODESIZE>);
        return "";
    }
)

(define_expand "extend<mode>di2"
    [(set (match_operand:DI 0 "register_operand" "")
	(sign_extend:DI (match_operand:QIHISI 1 "nonimmediate_operand" "")))]
    ""
    {
     if (ivm64_expand_unary(operands, <MODE>mode, DImode, SIGN_EXTEND))
        DONE;
    }
)

(define_insn "*extend<mode>di2"
    [(set (match_operand:DI 0 "tr_register_operand" "=r")
     (sign_extend:DI (match_operand:QIHISI 1 "tr_register_operand" "r")))]
    ""
    {
       ivm64_output_sign_extend(<MODESIZE>);
       return "";
    }
)


;;-------------------------------------------------------------------------
;; Branch and Jump instructions
;;-------------------------------------------------------------------------

(define_expand "cbranch<mode>4"
  [(set (pc)
        (if_then_else   (match_operator 0 "ordered_comparison_operator"
                        [   (match_operand:QIHISIDI 1 "general_operand" "")
                            (match_operand:QIHISIDI 2 "general_operand" "")])
                        (label_ref (match_operand 3 "" ""))
                        (pc)))]
  ""
  {
    if (!pushable_operand(operands[1], <MODE>mode))
        operands[1] = force_reg (<MODE>mode, operands[1]);
    if (!pushable_operand(operands[2], <MODE>mode))
        operands[2] = force_reg (<MODE>mode, operands[2]);
  }
)

(define_insn "cbranch<mode>4_internal"
  [(set (pc)
        (if_then_else   (match_operator 0 "ordered_comparison_operator"
                        [   (match_operand:QIHISIDI 1 "general_operand"
                                                      "i,i,i,St,St,St,m,m,m")
                            (match_operand:QIHISIDI 2 "general_operand"
                                                      "i,St,m,i,St,m,i,St,m")])
                        (label_ref (match_operand 3 "" ""))
                        (pc)))]
  ""
  {
        ivm64_output_cbranch(operands, <MODE>mode);
        return "";
  }
)

(define_insn "*peep2_cbranch<mode>"
  [(set (pc)
        (if_then_else   (match_operator 0 "ordered_comparison_operator"
                        [ (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
                          (match_operand:QIHISIDI 2 "arithmetic_operand" "iStm") ])
                        (label_ref (match_operand 1 "" ""))
                        (pc))
  )]
    ""
    {
        ivm64_output_cbranch_peep(operands, <MODE>mode, 0, insn);
        return "";
    }
)

(define_insn "*peep2_cbranch<mode>_reverse"
  [(set (pc)
        (if_then_else   (match_operator 0 "ordered_comparison_operator"
                        [ (match_operand:QIHISIDI 2 "arithmetic_operand" "iStm")
                          (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)])
                        (label_ref (match_operand 1 "" ""))
                        (pc))
  )]
    ""
    {
        ivm64_output_cbranch_peep(operands, <MODE>mode, 1, insn);
        return "";
    }
)

(define_insn "jump"
    [(set (pc)
         (label_ref (match_operand 0 "" "")))]
     ""
     {
        return "jump! %0";
     }
 )

(define_insn "indirect_jump"
    [(set (pc) (match_operand:DI 0 "general_operand" "i,St,m"))]
    ""
    {
        ivm64_output_push(operands[0], DImode);
        return "jump";
    }
)

(define_expand "casesi"
  [(match_operand:SI 0 "general_operand"  "i,St,m") ; index to jump on
   (match_operand:SI 1 "immediate_operand" "i,i,i") ; lower bound
   (match_operand:SI 2 "immediate_operand" "i,i,i") ; range = upper - lower bound
   (match_operand:SI 3 "" "")                       ; table label
   (match_operand:SI 4 "" "")]                      ; out-of-range label
  ""
  {
    emit_jump_insn(gen_casesi_internal (operands[0], operands[1],
                           operands[2], operands[3], operands[4]));
    DONE;
  }
)

(define_insn "casesi_internal"
  [(set (pc)
        (if_then_else
          (leu (minus:DI
                    (match_operand 0 "" "")
                    (match_operand:SI 1 "" ""))
              (match_operand:SI 2 "" ""))
          (mem:DI (plus:DI (label_ref (match_operand 3 "" ""))
                           (mult:DI (minus:DI (match_dup 0)
                                              (match_dup 1))
                                    (const_int 8))))
          (label_ref:DI (match_operand 4 "" ""))))
  ]
  ""
  {
    ivm64_output_casesi(operands);
    return "";
  }
)


;;-------------------------------------------------------------------------
;; Call instructions
;;-------------------------------------------------------------------------

(define_expand "call_pop"
  [(parallel [(call (match_operand:DI 0 "memory_operand" "S,m")
                     (match_operand:DI 1 "general_operand" "g,g"))
               (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                    (match_operand:DI 3 "immediate_operand" "i,i")))]
  )]
  ""
  {
      ivm64_expand_call_pop(operands);
      DONE;
  }
)

(define_insn "*call_pop"
  [(call (match_operand:DI 0 "memory_operand" "S,m")
         (match_operand:DI 1 "general_operand" "g,g"))
   (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                (match_operand:DI 2 "immediate_operand" "i,i"))
  )]
  ""
  {
      ivm64_output_call(operands[0]);
      if (! is_noreturn(insn)) {
          ivm64_output_setsp(asm_out_file, INTVAL(operands[2])/UNITS_PER_WORD);
      }
      ivm64_gpr_offset -= INTVAL (operands[2]);
      return "";
  }
)

(define_expand "call_value_pop"
  [(parallel [(set (match_operand 0 "gp_register_operand" "=r,r")
                   (call (match_operand:DI 1 "memory_operand" "S,m")
                         (match_operand:DI 2 "general_operand" "g,g")))
              (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                              (match_operand:DI 4 "immediate_operand" "i,i")))]
  )]
  ""
  {
      ivm64_expand_call_value_pop(operands);
      DONE;
  })


(define_insn "*call_value_pop"
  [(set (match_operand 0 "gp_register_operand" "=r,r")
        (call (match_operand:DI 1 "memory_operand" "S,m")
              (match_operand:DI 2 "general_operand" "g,g")))
    (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                               (match_operand:DI 3 "immediate_operand" "i,i")))
  ]
  ""
  {
      ivm64_output_call(operands[1]);
      long restored_slots = 0;
      ;// In these case ignore the return value
      if (!(is_a_push_memcpy(operands[3])
            || return_value_unused(insn))){
          restored_slots = ivm64_output_return_value(operands, 0);
      }
      ;// Pop remaining arguments after moving the return value
      ;// But if it is no return, print nothing beyond this point
      long npopargs = INTVAL(operands[3]) - restored_slots * UNITS_PER_WORD;
      if (npopargs > 0 && !is_noreturn(insn)) {
          ivm64_output_setsp(asm_out_file, npopargs/UNITS_PER_WORD);
      }
      ivm64_gpr_offset -= INTVAL (operands[3]);
      return "";
  }
)

(define_expand "call"
    [(call (match_operand:DI 0 "" "")
           (match_operand:DI 1 "" ""))]
    ""
    {
        ivm64_expand_call(operands);
        DONE;
    }
)

(define_insn "*call"
    [(call (match_operand:DI 0 "memory_operand" "S,m")
           (match_operand:DI 1 "general_operand" "g,g"))]
    ""
    {
        ivm64_output_call(operands[0]);
        if (is_noreturn(insn)){
           ivm64_gpr_offset -= INTVAL (operands[1]);
        }
        return "";
    }
)

(define_expand "call_value"
    [(set (match_operand 0 "gp_register_operand" "")
          (call (match_operand:DI 1 "" "")
                (match_operand:DI 2 "" "")))]
    ""
    {
        ivm64_expand_call_value(operands);
        DONE;
    }
)

(define_insn "*call_value"
    [(set (match_operand 0 "gp_register_operand" "=r,r")
          (call (match_operand:DI 1 "memory_operand" "S,m")
                (match_operand:DI 2 "general_operand" "g,g")))]
    ""
    {
        ivm64_output_call(operands[1]);
        ivm64_output_return_value(operands, 1);
        if (is_noreturn(insn)) {
           ivm64_gpr_offset -= INTVAL (operands[2]);
        }
        return "";
    }
)

;;-------------------------------------------------------------------------
;; NOP (no-op) instructions
;;-------------------------------------------------------------------------

(define_insn "blockage"
    [(unspec_volatile [(const_int 0)] UNSPEC_BLOCKAGE)]
    ""
    "")

(define_insn "nop"
    [(const_int 0)]
    ""
    "")

(define_insn "*clobber_tr"
    [(clobber (reg:DI TR_REGNUM))]
    ""
    "")


;;-------------------------------------------------------------------------
;; Builtins and stack allocation
;;-------------------------------------------------------------------------

(define_expand "call_alloca"
  [(match_operand 0 "general_operand" "") ;; place for storing the return value (to align it)
   (match_operand 1 "general_operand" "") ;; size (bytes) to be allocated
  ]
   "IVM64_NON_BUILTIN_ALLOCA"
   {
    ivm64_expand_call_alloca(operands);
    DONE;
  }
)

;; These two rules "save_stack_block" and "restore_stack_block" implement
;; how the stack is save and restored surounding the allocation for VLAs
;; In our case is implemented calling the functions that emulates this functionality
;; for the software alloca() call
(define_expand "save_stack_block"
  [(match_operand 0 "general_operand" "") ;; the returned pointer ??
   (match_operand 1 "general_operand" "") ;; stack pointer
  ]
  "IVM64_NON_BUILTIN_ALLOCA"
  {
    ivm64_expand_save_stack_block(operands);
    DONE;
  }
)

(define_expand "restore_stack_block"
  [(match_operand 0 "general_operand" "") ;; stack pointer
   (match_operand 1 "general_operand" "") ;; pointer where the stack was saved
  ]
  "IVM64_NON_BUILTIN_ALLOCA"
  {
    ivm64_expand_restore_stack_block(operands);
    DONE;
  }
)

(define_expand "builtin_longjmp"
  ;; only using the first argument; the second one is always 1
  [(match_operand 0 "general_operand" "")]
   ""
  {
    ivm64_expand_builtin_longjump_inline(operands);
    DONE;
  }
)

;; used by __builtin_longjmp
(define_insn "*call_nop"
   [(call (unspec_volatile [(const_int 0)] UNSPEC_BLOCKAGE) (const_int 0))]
   ""
   ""
)

;; Define nonlocal_gotos to be able to jump to outer labels when
;; declaring nested functions
(define_expand "nonlocal_goto"
  [(set (pc)
	(unspec_volatile [(match_operand 0 "") ;; fp (ignore)
			  (match_operand 1 "") ;; target
			  (match_operand 2 "") ;; sp
			  (match_operand 3 "") ;; ?
			  ] UNSPEC_NONLOCAL_GOTO))
   ]
  ""
  "emit_jump_insn (gen_nonlocal_goto_insn (operands[0], operands[1], operands[2], operands[3]));
   emit_barrier ();
   DONE;"
  )

(define_insn "nonlocal_goto_insn"
  [(set (pc)
	(unspec_volatile [(match_operand 0 "" "") ;; fp (ignore)
			  (match_operand 1 "" "im") ;; target
			  (match_operand 2 "" "im") ;; sp
			  (match_operand 3 "" "im") ;; ?
			  ] UNSPEC_NONLOCAL_GOTO))
   ]
  ""
  {
    ivm64_output_push(operands[2], DImode);
    ivm64_output_pop(stack_pointer_rtx, DImode);

    ivm64_output_push(operands[1], DImode);
    output_asm_insn ("jump", NULL);

    return "";
  }
)



;;-------------------------------------------------------------------------
;; Patterns for special use like peepholes
;;-------------------------------------------------------------------------

(define_insn "*peep2_mov_from_mem_tr_to_tr_ind_push<mode>"
  [(set (reg:ALLMODES TR_REGNUM)
        (unspec_volatile:ALLMODES
                     [(mem:ALLMODES (reg:DI TR_REGNUM))] UNSPEC_IND_PUSH_TR)
    )]
    ""
    "load<MODESIZE>"
)

(define_insn "*peep2_mov_from_mem_tr_to_tr_ind_offset_push<mode>"
  [(set (reg:ALLMODES TR_REGNUM)
        (unspec_volatile:ALLMODES
           [(mem:ALLMODES (plus:DI (reg:DI TR_REGNUM)
                   (match_operand:DI 0 "" "i")) )] UNSPEC_IND_OFFSET_PUSH_TR)
    )]
    ""
    "add! %0\;load<MODESIZE>"
)

(define_insn "*peep2_mov_from_tr_to_mem_tr_ind_pop<mode>"
  [(set (mem:ALLMODES (reg:DI TR_REGNUM))
        (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_IND_POP_TR)
    )]
    ""
    {
        emitted_pop_cfun += 2;
        return "store<MODESIZE>";
    }
)

(define_insn "*peep2_pop_pusharg"
  [
   (unspec_volatile:ALLMODES [(mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
                        (reg:ALLMODES TR_REGNUM)] UNSPEC_POP_PUSHARG_TR)
  ]
    ""
  {
    emitted_pop_cfun++;
    ivm64_gpr_offset += UNITS_PER_WORD;
    return "";
  }
)

(define_insn "*binop_tr_tr"
   [
     (set (reg:DI TR_REGNUM)
          (match_operator:DI 0 "binary_operator"
                 [(unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
                  (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)])
     )
   ]
   ""
   {
        emitted_pop_cfun++;
        return ivm64_rtxop2insn(operands[0]);
   }
)

(define_insn "*jump_tr"
   [
     (set (pc) (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR))
   ]
   ""
   {
     emitted_pop_cfun++;
     return "jump";
   }
)

(define_insn "*neg_tr"
   [
     (set (reg:DI TR_REGNUM)
          (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_NEG_TR))
   ]
   ""
   "neg"
)

;; This represents replacing the top of the stack
;; by 2^(top of the stack)
(define_insn "*pow_tr"
   [
     (set (reg:DI TR_REGNUM)
          (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POW_TR))
   ]
   ""
   "pow2"
)

(define_insn "*set_sp_tr"
   [
     (unspec_volatile:DI [(const_int 0)] UNSPEC_POP_SP)
   ]
   ""
   {
     emitted_pop_cfun++;
     return "set_sp";
   }
)

(define_insn "print_asm"
  [ (unspec_volatile:DI [(match_operand 0 "" "")] UNSPEC_PRINT)]
  ""
  {
    fprintf(asm_out_file, "%s\n", XSTR(operands[0],0));
    return "";
  }
)

(define_insn "nop_push"
  [(unspec_volatile [(const_int 100)] UNSPEC_NOP_PUSH_TR)]
  ""
  {
    emitted_push_cfun++;
    return "";
  }
)

(define_insn "nop_pop"
  [(unspec_volatile [(const_int 101)] UNSPEC_NOP_POP_TR)]
  ""
  {
    emitted_pop_cfun++;
    return "";
  }
)

(define_insn "nop_ar_offset"
  [(unspec_volatile
      [(match_operand 0 "const_int_operand" "i")] UNSPEC_AR_OFFSET)]
  ""
  {
    ivm64_gpr_offset += INTVAL(operands[0]);
    return "";
  }

)

(define_insn "nop_extra_offset"
  [(unspec_volatile
      [(match_operand 0 "const_int_operand" "i")] UNSPEC_EXTRA_OFFSET)]
  ""
  {
    ivm64_stack_extra_offset += INTVAL(operands[0]);
    return "";
  }
)

;;-------------------------------------------------------------------------
;; Peephole optimization
;;-------------------------------------------------------------------------

(include "peephole.md")

