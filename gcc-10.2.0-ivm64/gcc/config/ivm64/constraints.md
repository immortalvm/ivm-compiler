;; Preservation Virtual Machine Project
;; Constraint definitions for the ivm64 target
;;
;; Authors:
;;  Eladio Gutierrez Carrasco
;;  Sergio Romero Montiel
;;  Oscar Plata Gonzalez
;;
;; Date: Oct 2019

;; Check 16.8.7 Defining Machine-Specific Constraints for details
;; https://gcc.gnu.org/onlinedocs/gcc-8.3.0/gccint/Define-Constraints.html#Define-Constraints

;; NO contraints can be prefixed with: E F V X g i m n o p r s

(define_register_constraint "t" "GP_REG" "General purpose registers")

(define_constraint "S"
  "Operands involving SP"
  (match_test
     "(
         (REG_P(op) && (REGNO(op) == STACK_POINTER_REGNUM ))
          || (REG_P(op) && REGNO(op) > LAST_VIRTUAL_REGISTER)
          || (GET_CODE(op) == MEM && PC_OR_STACK_RELATIVE_P(XEXP(op, 0)))
      )")
)

(define_constraint "Q"
  "As S but excluding SP"
  (match_test
     "(
          (REG_P(op) && REGNO(op) > LAST_VIRTUAL_REGISTER)
          || (GET_CODE(op) == MEM && PC_OR_STACK_RELATIVE_P(XEXP(op, 0)))
      )")
)
