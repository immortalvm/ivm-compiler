;; Preservation Virtual Machine Project
;; Operand and operator predicates for the ivm64 target
;;
;; Authors:
;;  Eladio Gutierrez Carrasco
;;  Sergio Romero Montiel
;;  Oscar Plata Gonzalez
;;
;; Date: Oct 2019 - Dec 2020

;; Remember that predicate names must end with "_operand"

(define_predicate "sp_register_operand"
   (and (match_code "reg" )
        (match_test "(REGNO(op) == STACK_POINTER_REGNUM
                      && GET_MODE(op) == mode)"
                    )
               )
)

(define_predicate "tr_register_operand"
   (and (match_code "reg" )
        (match_test "(REGNO(op) == TR_REGNUM
                      && (mode == VOIDmode
                         || GET_MODE(op) == mode))"
                    )
               )
)

(define_predicate "gp_register_operand"
   (and (match_code "reg" )
        (match_test "(IS_GP_REGNUM(REGNO(op))
                      && (mode == VOIDmode
                         ||GET_MODE(op) == mode))"
                    )
               )
)

(define_predicate "general_not_fp_operand"
  (match_test "true")
  {
    return general_operand(op, mode)
         && !(REG_P(op) && (REGNO(op)== FRAME_POINTER_REGNUM));
  }
)

(define_predicate "general_not_sp_operand"
  (match_test "true")
  {
    return general_operand(op, mode)
         && !(REG_P(op) && (REGNO(op)== STACK_POINTER_REGNUM));
  }
)

(define_predicate "general_not_tr_operand"
  (match_test "true")
  {
    return general_operand(op, mode)
           && !(REG_P(op) && (REGNO(op) == TR_REGNUM));
  }
)

(define_predicate "general_dst_operand"
  (match_code "mem,reg,subreg")
  {
    return general_operand(op,mode);
  }
)

(define_predicate "general_dst_not_tr_operand"
  (match_code "mem,reg,subreg")
  {
    return general_not_tr_operand(op, mode);
  }
)

(define_predicate "general_dst_not_tr_sp_operand"
  (match_code "mem,reg,subreg")
  {
    return general_not_tr_operand(op, mode)
           && general_not_sp_operand(op, mode);
  }
)


;; An arithmetic operand can be printed as a simple argument
;; of an ivm arithmetic instruction like "add! &2".
;; This includes immediates, labels, GPRs and mem[SP+offset]
(define_predicate "arithmetic_operand"
  (match_test "true")
  {
    return immediate_operand(op, mode)
           || (REG_P(op) && (IS_GP_REGNUM(REGNO(op))
                             || REGNO(op) == TR_REGNUM
                             || (REGNO(op) > LAST_VIRTUAL_REGISTER)
                            ))
           || (GET_CODE(op) == MEM && PC_OR_STACK_RELATIVE_P(XEXP(op, 0)))
    ;
  })


(define_predicate "pushable_operand"
  (match_test "true")
{
  rtx addr = NULL;
  if (GET_CODE(op) == MEM) addr = XEXP(op,0);
  return satisfies_constraint_i(op)
         || satisfies_constraint_S(op)
         || REG_P(op)
         || (addr && PC_OR_STACK_RELATIVE_P(addr))
        ;
})


(define_predicate "popable_operand"
  (match_code "mem,reg,subreg")
{
  rtx addr = NULL;
  if (GET_CODE(op) == MEM) addr = XEXP(op,0);
  return satisfies_constraint_S(op)
         || REG_P(op)
         || (addr && PC_OR_STACK_RELATIVE_P(addr))
        ;
})


(define_predicate "binary_operator"
  (match_code "plus,minus,mult,div,mod,udiv,umod,ior,xor,and,lshiftrt,ashiftrt,ashift"))

(define_predicate "binary_not_shift_operator"
  (match_code "plus,minus,mult,div,mod,udiv,umod,ior,xor,and"))

(define_predicate "unary_operator"
  (match_code "sign_extend,zero_extend,not,neg"))

(define_predicate "aggregation_operator"
  (match_code "plus,mult,and,ior,xor"))
