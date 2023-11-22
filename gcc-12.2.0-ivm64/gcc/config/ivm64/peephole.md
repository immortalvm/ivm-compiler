;; Preservation Virtual Machine Project
;; Peephole definitions for the ivm64 target
;;
;; Authors:
;;  Eladio Gutierrez Carrasco
;;  Sergio Romero Montiel
;;  Oscar Plata Gonzalez
;;
;; Date: Apr-Jul 2019 - May 2021

;; -------------------------------------------------------------------------
;; Peepholes
;; -------------------------------------------------------------------------

;; There are two forms of peephole definitions that may be used. The
;; original define_ peephole is run at assembly output time to match insns and
;; substitute assembly text. Use of define_peephole is deprecated.
;; A newer define_peephole2 matches insns and substitutes new insns.
;; The peephole2 pass is run after register allocation but before scheduling,
;; which may result in much  better code for targets that do scheduling.

;; -----------------------------------------------
;; TR <- r0 (r0 dead)
;; r0 <- TR
;;       => NOP
;; -----------------------------------------------
(define_peephole2
  [
   (set (reg:ALLMODES TR_REGNUM)
             (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                               (match_operand:ALLMODES 0 "gp_register_operand" "")]
              UNSPEC_PUSH_TR)
     )
   (set (match_dup:ALLMODES 0)
             (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
     )
  ]
  "
   ivm64_peep_enabled(IVM64_PEEP2_PUSH_POP)
   && peep2_reg_dead_p(1, operands[0])
  "
  [
     (const_int 0)  ;; nop
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- r1
;; r0 <- r2
;;       =>  r0 <- r2
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:ALLMODES 0 "general_dst_not_tr_operand" "")
         (match_operand:ALLMODES 1 "general_not_tr_operand" "")
    )
    ;;---
    (set (match_dup:ALLMODES 0)
         (match_operand:ALLMODES 2 "general_not_tr_operand" "")
   )
  ]
  "ivm64_peep_enabled(IVM64_PEEP2_PUSH_POP_PUSH_POP)"
  [
    (set (match_dup:ALLMODES 0) (match_dup:ALLMODES 2))
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- p1 (DI)
;; p2 <- r0 (QI,SI,...) (r0 dead && p1, p2 not depending on r0)
;;       => TR <- p1
;;          p2 <- TR(QI, SI, ...)
;;
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:ALLMODES1 0 "gp_register_operand" "")
         (match_operand:ALLMODES1 1 "pushable_operand" "")
    )
    (set (match_operand:ALLMODES2 2 "popable_operand" "")
         (match_operand:ALLMODES2 3 "gp_register_operand")
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOVE_MOVE)
    && (REGNO(operands[0]) == REGNO(operands[3]))
    && ! reg_mentioned_p(operands[0], operands[1])
    && ! reg_mentioned_p(operands[0], operands[2])
    && peep2_reg_dead_p(2, operands[0])
  )"
  [
    ;;---
    (set (reg:ALLMODES1 TR_REGNUM)
         (unspec_volatile:ALLMODES1 [(reg:ALLMODES1 TR_REGNUM)
                                (match_dup:ALLMODES1 1)]
               UNSPEC_PUSH_TR)
      )
    (set (match_operand:ALLMODES2 2)
         (unspec_volatile:ALLMODES2 [(reg:ALLMODES2 TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---

  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- r1 (QI,SI,...)  (r1 != r0)
;; r2 <- TR (size conversion) (r2 != r0)
;; TR <- r0 (r0 dead)
;;          => TR <- r1 (QI,SI,...)
;;             r1 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
             (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
             (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                               (match_operand:ALLMODES 1 "gp_register_operand" "")]
             UNSPEC_PUSH_TR)
    )
    ;;---
    ;;---
    (set (match_operand:DI 2 "register_operand" "")
             (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
             (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                  (match_dup:DI 0)]
              UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_PUSH_POP_PUSH)
    && (REGNO(operands[1]) == REGNO(operands[2]))
    && (REGNO(operands[0]) != REGNO(operands[1]))
    && peep2_reg_dead_p(4,operands[0]) )
  "
  [
    (set (reg:ALLMODES TR_REGNUM)
             (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                               (match_dup:ALLMODES 1)]
             UNSPEC_PUSH_TR)
    )
    ;;---
    ;;---
    (set (match_dup:DI 2)
             (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; r1 <- r0 (QI,SI,...) (r0 dead)
;;       =>  r0 <- TR (QI,SI,...)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (match_operand:ALLMODES 1 "gp_register_operand" "")
         (match_operand:ALLMODES 2 "register_operand" "")
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_MOVE)
    && (REGNO(operands[0]) == REGNO(operands[2]))
    && (REGNO(operands[1]) != REGNO(operands[2]))
    && peep2_reg_dead_p(2, operands[0]))"
  [
    (set (match_dup:ALLMODES 1)
             (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
  ]
  {
  }
  )

;; -----------------------------------------------
;; r0 <- TR
;; c <- r0 (QI,...) (r0 dead, c not depending on r0)
;;      =>  c <- TR (QI,...)
;; -----------------------------------------------
(define_peephole2
  [
    ;--
    (set (match_operand:ALLMODES1 0 "gp_register_operand" "")
         (unspec_volatile:ALLMODES1 [(reg:ALLMODES1 TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;--
    (set (match_operand:ALLMODES2 1 "popable_operand" "")
         (match_operand:ALLMODES2 2 "gp_register_operand" ""))
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_MOVE2)
    && peep2_reg_dead_p(2, operands[2])
    && (REGNO(operands[0]) == REGNO(operands[2]))
    && ! reg_mentioned_p(operands[0], operands[1]))
  "
  [
    ;;---
    (set (match_dup:ALLMODES2 1)
         (unspec_volatile:ALLMODES2 [(reg:ALLMODES2 TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  {
  }
  )

;; -----------------------------------------------
;; r0 <- TR (QI, ...)
;; mem[--SP] <- r0 (r0 dead)
;;              => TR <- zero_extend(TR) (QI,HI,SI to DI)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:ALLMODES 0 "gp_register_operand" "")
         (unspec_volatile:ALLMODES [
                  (reg:ALLMODES TR_REGNUM)
              ] UNSPEC_POP_TR))
    ;;---
    (set (mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
         (match_dup:ALLMODES 0)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_PUSHARG)
    && peep2_reg_dead_p(2, operands[0])
   )"
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (zero_extend:DI (reg:ALLMODES TR_REGNUM))
    )
    ;;---
     (unspec_volatile:ALLMODES [(mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
                          (reg:ALLMODES TR_REGNUM)] UNSPEC_POP_PUSHARG_TR)
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- r1 (QI, ...)
;; mem[--SP] <- r0 (r0 dead reg)
;;              => mem[-SP] <- r1 (QI, ...)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
      (set (match_operand:ALLMODES 0 "gp_register_operand" "")
           (match_operand:ALLMODES 1 "pushable_operand" "")
      )
    ;;---
      (set (mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
          (match_dup:ALLMODES 0)
      )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOVE_PUSHARG)
    && peep2_reg_dead_p(2, operands[0])
  )"
  [
      (set (mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
          (match_dup:ALLMODES 1)
      )
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- r1
;; TR <- r1
;; TR <- TR binop r0 (r0 dead)
;; r1 <- TR
;;       =>  TR <- r1
;;           TR <- TR binop r1
;;           r1  <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
              (match_operand:DI 1 "gp_register_operand" "")
        )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup 1)]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 2 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_dup:DI 0)])
    )
    (set (match_dup:DI 1)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOVE_BINOP)
    && peep2_reg_dead_p(3, operands[0])
   )"
  [
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 1)]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 2 [(reg:DI TR_REGNUM)
                  (match_dup:DI 1)])
    )
    (set (match_dup:DI 1)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- s1  (s1 is commonly SP)
;; TR <- r0
;; TR <- TR binop s2 (s2 not depending on r0 )
;; r0 <- TR
;;       => TR <- s1
;;          TR <- TR binop s2
;;          r0 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:ALLMODES 0 "gp_register_operand" "")
              (match_operand:ALLMODES 1 "general_not_tr_operand" "")
        )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_dup:ALLMODES 0)]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 3 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 2 "arithmetic_operand" "")])
    )
    (set (match_dup:ALLMODES 0)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_BINOP1)
    && ! reg_mentioned_p(operands[0], operands[2])
   )"
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_dup:ALLMODES 1)]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 3 [(reg:DI TR_REGNUM)
                  (match_dup:DI 2)])
    )
    (set (match_dup:ALLMODES 0)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- a
;; TR <- TR binop1 x
;; a <- TR
;; TR <- a
;; TR <- TR binop2 y
;; a <- TR
;; TR <- a
;; TR <- TR binop3 z
;; a <- TR
;;      =>  TR <- a
;;          TR <- TR binop1 x
;;          TR <- TR binop2 y
;;          TR <- TR binop3 z
;;          a <- TR
;; -----------------------------------------------
  (define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                               (match_operand:DI 0 "general_dst_operand" "")]
          UNSPEC_PUSH_TR)
      )

    (set (reg:DI TR_REGNUM)
         (match_operator:DI 4 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "arithmetic_operand" "")])
    )
    (set (match_dup:DI 0)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                              (match_dup:DI 0)]
          UNSPEC_PUSH_TR)
    )

    (set (reg:DI TR_REGNUM)
         (match_operator:DI 5 "binary_operator" [(reg:DI TR_REGNUM)
               (match_operand:DI 2 "arithmetic_operand" "")])
    )
    (set (match_dup:DI 0)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 0)]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 6 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 3 "arithmetic_operand" "")])
    )
    ;;---
    (set (match_dup:DI 0)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_BINOP_1_2_3)
    &&  ! reg_mentioned_p(operands[0], operands[2])
    &&  ! reg_mentioned_p(operands[0], operands[3])
  )"
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 0)]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 4 [(reg:DI TR_REGNUM)
                  (match_dup:DI 1)] )
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 5 [(reg:DI TR_REGNUM)
                  (match_dup:DI 2)] )
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 6 [(reg:DI TR_REGNUM)
                  (match_dup:DI 3)])
    )
    ;;---
    (set (match_dup:DI 0)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p0
;; TR <- TR binop5 p1
;; r2 <- TR
;; TR <- r2 (r2 dead)
;; TR <- TR binop6 p3
;; p4 <- TR
;;         =>  TR <- p0
;;             TR <- TR binop5 p1
;;             TR <- TR binop6 p3
;;             p4 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                            (match_operand:DI 0 "general_operand" "")]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 5 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "arithmetic_operand" "")] )
    )
    (set (match_operand:DI 2 "register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                               (match_dup:DI 2 )]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 6 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 3 "arithmetic_operand" "")] )
    )
    ;;---
    (set (match_operand:DI 4 "general_dst_operand")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_BINOP_1_DEADREG_BINOP_2)
    && peep2_reg_dead_p(4, operands[2])
    && ! reg_mentioned_p(operands[2], operands[3])
    && (REG_P(operands[4]) || !reg_mentioned_p(operands[2], operands[4]))
   )"
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                            (match_dup:DI 0)]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 5 [(reg:DI TR_REGNUM)
                  (match_dup:DI 1)] )
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 6 [(reg:DI TR_REGNUM)
                  (match_dup:DI 3)] )
    )
    ;;---
    (set (match_dup:DI 4)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p0
;; TR <- TR binop4 p1
;; p2 <- TR
;; TR <- p2
;; TR <- TR binop5 p3
;; p2 <- TR
;; =>  TR <- p0
;;     TR <- TR binop4 p1
;;     TR <- TR binop5 p3 (p3 not depending on p2)
;;     p2 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand:DI 0 "pushable_operand" "")]
               UNSPEC_PUSH_TR)
      )

    (set (reg:DI TR_REGNUM)
         (match_operator:DI 4 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "arithmetic_operand" "")] )
    )
    (set (match_operand:DI 2 "general_dst_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 2)]
               UNSPEC_PUSH_TR)
      )

    (set (reg:DI TR_REGNUM)
         (match_operator:DI 5 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 3 "arithmetic_operand" "")] )
    )
    (set (match_dup:DI 2)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
      )
    ;;---
  ]
  "ivm64_peep_enabled(IVM64_PEEP2_BINOP_1_2)
    && ! reg_mentioned_p(operands[2], operands[3])
  "
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                            (match_dup:DI 0)]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 4 [(reg:DI TR_REGNUM)
                  (match_dup:DI 1)] )
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 5 [(reg:DI TR_REGNUM)
                  (match_dup:DI 3)] )
    )
    ;;---
    (set (match_dup:DI 2)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p0
;; TR <- TR binop1 p1
;; TR <- TR binop2 p2
;; r3 <- TR
;; TR <- r3 (r3 dead)
;; TR <- TR binop3 p4
;; p5 <- TR
;;         =>  TR <- p0
;;             TR <- TR binop1 p2
;;             TR <- TR binop2 p2
;;             TR <- TR binop3 p4
;;             p5 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                        (match_operand:DI 0 "general_operand" "")]
           UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 6 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "arithmetic_operand" "")] )
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 7 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 2 "arithmetic_operand" "")] )
    )
    (set (match_operand:DI 3 "register_operand")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                               (match_dup:DI 3 )]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 8 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 4 "arithmetic_operand" "")] )
    )
    (set (match_operand:DI 5 "general_dst_operand")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_BINOP_1_2_DEADREG_BINOP_3)
    && peep2_reg_dead_p(5, operands[3])
    && !reg_mentioned_p(operands[3], operands[4])
    && (REG_P(operands[5]) || !reg_mentioned_p(operands[3], operands[5]))
  )"
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                            (match_dup:DI 0)]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 6 [(reg:DI TR_REGNUM)
                  (match_dup:DI 1)] )
    )
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 7 [(reg:DI TR_REGNUM)
                  (match_dup:DI 2)] )
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 8 [(reg:DI TR_REGNUM)
                  (match_dup:DI 4)] )
    )
    ;;---
    (set (match_operand:DI 5 "general_dst_operand")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p0 (QI, ...)
;; TR <- zero_extend(TR) (QI, ...)
;; (p1 <- TR (DI))
;;           =>  TR(SI...) <- p0 (QI, ...)
;;               (p1 <- TR (DI))
;; -----------------------------------------------
(define_peephole2
  [
    ;--
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI[(reg:QIHISI TR_REGNUM)
                            (match_operand:QIHISI 0 "general_operand" "")]
           UNSPEC_PUSH_TR)
    )
    ;--
    (set (reg:DI TR_REGNUM)
         (zero_extend:DI (reg:QIHISI TR_REGNUM))
    )
    ;--
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_ZERO_EXTEND))"
  [
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI[(reg:QIHISI TR_REGNUM)
                            (match_dup:QIHISI 0)]
           UNSPEC_PUSH_TR)
    )
    ;;--
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p0 (QI, ...)
;; r1 <- TR
;; TR <- r1 (r1 dead)
;; TR <- TR binop p2
;; p3 <- TR
;;      =>  TR <- p0 (QI, ...)
;;          TR <- extend TR
;;          TR <- TR binop p2
;;          p3 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;--
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (match_operand:ALLMODES 0 "general_operand" "")]
            UNSPEC_PUSH_TR)
      )
    ;;---
    (set (match_operand:DI 1 "register_operand")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                            (match_dup:DI 1) ]
           UNSPEC_PUSH_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 4 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 2 "arithmetic_operand" "")] )
    )
    (set (match_operand:DI 3 "general_dst_operand")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
      )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_PUSH_POP_DEADREG_BINOP)
    && peep2_reg_dead_p(3, operands[1])
    && !reg_mentioned_p(operands[1], operands[2])
    && (REG_P(operands[3]) || !reg_mentioned_p(operands[1], operands[3]))
  )"
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_dup:ALLMODES 0)]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 4 [(reg:DI TR_REGNUM)
                  (match_dup:DI 2)] )
    )
    ;;---
    (set (match_dup:DI 3)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p0 (QI...)
;; TR <- unary(TR) (QI...) (usually sign_extend)
;; r1 <- TR
;; TR <- r1 (r1 dead)
;; TR <- TR binop p3
;; p4 <- TR
;;       =>  TR <- p0
;;           TR <- unary(TR)
;;           TR <- TR binop p3
;;           p4 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;--
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_operand:ALLMODES 0 "general_operand" "")]
               UNSPEC_PUSH_TR)
      )
    ;--
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 5 "unary_operator" [(reg:ALLMODES TR_REGNUM)])
    )
    (set (match_operand:DI 1 "register_operand")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 1) ]
               UNSPEC_PUSH_TR)
      )
    ;;--
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 6 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 3 "arithmetic_operand" "")] )
    )
    (set (match_operand:DI 4 "general_dst_operand")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
      )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_UNARY_DEADREG_BINOP)
    && peep2_reg_dead_p(4, operands[1])
    && ! reg_mentioned_p(operands[1], operands[3])
    && (REG_P(operands[4]) || ! reg_mentioned_p(operands[1], operands[4]))
  )"
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_dup:ALLMODES 0)]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 5 [(reg:ALLMODES TR_REGNUM)])
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 6 [(reg:DI TR_REGNUM)
                  (match_dup:DI 3)] )
    )
    ;;---
    (set (match_dup:DI 4)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p0 (QI...)
;; TR <- unary(TR) (QI...)  (usually sign_extend)
;; TR <- TR binop6 p1
;; r2 <- TR
;; TR <- r2 (r2 dead)
;; TR <- TR binop7 p3
;; p4 <- TR
;;         =>  TR <- p0
;;             TR <- unary TR
;;             TR <- TR binop6 p1
;;             TR <- TR binop7 p3
;;             p4 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_operand:ALLMODES 0 "general_operand" "")]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 5 "unary_operator" [(reg:ALLMODES TR_REGNUM)])
    )
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 6 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "arithmetic_operand" "")] )
    )
    ;;---
    (set (match_operand:DI 2 "register_operand")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                   (match_dup:DI 2 )]
               UNSPEC_PUSH_TR)
      )

    (set (reg:DI TR_REGNUM)
         (match_operator:DI 7 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 3 "arithmetic_operand" "")] )
    )
    (set (match_operand:DI 4 "general_dst_operand")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
      )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_UNARY_1_BINOP_2_DEADREG_BINOP_3)
    && peep2_reg_dead_p(5, operands[2])
    && ! rtx_equal_p(operands[2], operands[3])
    && (REG_P(operands[4]) || ! reg_mentioned_p(operands[2], operands[4]))
  )"
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_dup:ALLMODES 0)]
               UNSPEC_PUSH_TR)
      )

    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 5 [(reg:ALLMODES TR_REGNUM)] )
    )
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 6 [(reg:DI TR_REGNUM)
                  (match_dup:DI 1)] )
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 7 [(reg:DI TR_REGNUM)
                  (match_dup:DI 3)] )
    )
    (set (match_operand:DI 4 "general_dst_operand")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- TR + x
;; TR <- TR + y
;;         =>  TR <- TR + (x+y)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 3 "aggregation_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "const_int_operand" "")] )
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 4 "aggregation_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 2 "const_int_operand" "")] )
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_RDX_1_2)
    && (GET_CODE(operands[3]) == GET_CODE(operands[4]))
   )"
  [
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 3 [(reg:DI TR_REGNUM)
                  (match_dup:DI 1)] )
    )
  ]
  {
      if (GET_CODE(operands[3]) == PLUS){
          operands[1] = GEN_INT(INTVAL(operands[1]) + INTVAL(operands[2]));
      } else if (GET_CODE(operands[3]) == MULT) {
          operands[1] = GEN_INT(INTVAL(operands[1]) * INTVAL(operands[2]));
      } else if (GET_CODE(operands[3]) == AND) {
          operands[1] = GEN_INT(INTVAL(operands[1]) & INTVAL(operands[2]));
      } else if (GET_CODE(operands[3]) == IOR) {
          operands[1] = GEN_INT(INTVAL(operands[1]) | INTVAL(operands[2]));
      } else if (GET_CODE(operands[3]) == XOR) {
          operands[1] = GEN_INT(INTVAL(operands[1]) ^ INTVAL(operands[2]));
      }
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- r0 (QI, ...) (r dead)
;; TR <- sigx(TR) (QI, ...)
;;            =>  TR <- sigx(TR) (QI, ...)
;; -----------------------------------------------
(define_peephole2
  [
    ;--
    (set (match_operand:DI 0 "gp_register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)
                           (match_operand:QIHISI 1 "gp_register_operand" "")]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (reg:QIHISI TR_REGNUM))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_SIGNX)
    && (REGNO(operands[0]) == REGNO(operands[1]))
    && peep2_reg_dead_p(2, operands[1]))"
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (reg:QIHISI TR_REGNUM))
    )
    ;;---
  ]
    {
    }
)

;; -----------------------------------------------
;;  r0  <- r1
;;  TR  <- r2
;;  TR <- TR commutative binop r0 (r0 dead)
;;        =>  TR <- r2
;;            TR <- TR commutative binop r1
;; -----------------------------------------------
(define_peephole2
  [
  ;;--
  (set (match_operand:DI 0 "gp_register_operand" "")
       (match_operand:DI 1 "gp_register_operand" "")
  )
  ;;--
  (set (reg:DI TR_REGNUM)
       (unspec_volatile:DI [(reg:DI TR_REGNUM)
                          (match_operand:DI 2 "register_operand" "")]
         UNSPEC_PUSH_TR)
  )
  ;;--
  (set (reg:DI TR_REGNUM)
       (match_operator:DI 3 "aggregation_operator" [(reg:DI TR_REGNUM)
                                                    (match_dup:DI 0)])
  )
  ;;--
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOVE_PUSH_COMMUTATIVE)
    && peep2_reg_dead_p(3, operands[0])
    && (REGNO(operands[0]) != REGNO(operands[2]))
  )"
  [
  ;;--
  (set (reg:DI TR_REGNUM)
       (unspec_volatile:DI [(reg:DI TR_REGNUM)
                          (match_dup:DI 2)]
         UNSPEC_PUSH_TR)
  )
  ;;--
  (set (reg:DI TR_REGNUM)
       (match_op_dup:DI 3 [(reg:DI TR_REGNUM) (match_dup:DI 1)])
  )
  ;;--
  ]
  {
  }
)

;; -----------------------------------------------
;;  r0  <- imm1
;;  TR  <- r2
;;  TR <- TR commutative binop r0 (r0 dead)
;;        =>  TR <- r2
;;            TR <- TR commutative binop imm1
;; -----------------------------------------------
(define_peephole2
  [
  ;;--
  (set (match_operand:DI 0 "gp_register_operand" "")
       (match_operand:DI 1 "immediate_operand" "")
  )
  ;;--
  (set (reg:DI TR_REGNUM)
       (unspec_volatile:DI [(reg:DI TR_REGNUM)
                          (match_operand:DI 2 "register_operand" "")]
         UNSPEC_PUSH_TR)
  )
  ;;--
  (set (reg:DI TR_REGNUM)
       (match_operator:DI 3 "aggregation_operator" [(reg:DI TR_REGNUM) (match_dup:DI 0)])
  )
  ;;--
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOVE_PUSH_IMM_COMMUTATIVE)
    && peep2_reg_dead_p(3, operands[0]))"
  [
  ;;--
  (set (reg:DI TR_REGNUM)
       (unspec_volatile:DI [(reg:DI TR_REGNUM)
                          (match_dup:DI 2)]
         UNSPEC_PUSH_TR)
  )
  ;;--
  (set (reg:DI TR_REGNUM)
       (match_op_dup:DI 3 [(reg:DI TR_REGNUM) (match_dup:DI 1)])
  )
  ;;--
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- s1
;; TR <- TR - r0 (r0 dead, s1 not depending on r0)
;; r3 <- TR
;;       => TR <- s1 - TR = -(TR + s1)
;;          r3 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                   (match_operand:DI 1 "arithmetic_operand" "")]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (minus:DI (reg:DI TR_REGNUM) (match_dup:DI 0))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_PUSH_SUB)
    && ! reg_mentioned_p(operands[0], operands[1])
    && peep2_reg_dead_p(3, operands[0])
   )"
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (minus:DI (reg:DI TR_REGNUM) (match_dup:DI 1))
    )
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_NEG_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- r1
;; TR <- s2
;; TR <- TR - r0 (r0 dead, s2 not depending on r0)
;; r3 <- TR
;;       => TR <- r1
;;          TR <- s2 - TR = -(TR + s2)
;;          r3 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
         (match_operand:DI 1 "gp_register_operand" "")
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                               (match_operand:DI 2 "arithmetic_operand" "")]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (minus:DI (reg:DI TR_REGNUM) (match_dup:DI 0))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOVE_PUSH_SUB)
    && ! reg_mentioned_p(operands[0], operands[2])
    && peep2_reg_dead_p(3, operands[0])
   )"
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)
                               (match_dup:DI 1)]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (minus:DI (reg:DI TR_REGNUM) (match_dup:DI 2))
    )
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_NEG_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- s1 (QI, ...)
;; TR <- TR + r0 (r0 dead AND r0 not in s1)
;; r3 <- TR
;;       =>  TR <- s1 + TR = TR + s1
;;           r3 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
         (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                               (match_operand:QIHISIDI 1 "arithmetic_operand" "")]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 2 "aggregation_operator" [(reg:DI TR_REGNUM)
                                                      (match_dup:DI 0)])
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_PUSH_COMMUTATIVE)
    && peep2_reg_dead_p(3, operands[0])
    && ! reg_mentioned_p(operands[0], operands[1])
  )"
  [
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
         (match_op_dup:QIHISIDI 2 [(reg:QIHISIDI TR_REGNUM) (match_dup:QIHISIDI 1)])
    )
    ;;)
  ]
  {
  }
)

;; -----------------------------------------------
;;  r0  <- p1 (QI,...)
;;  TR  <- r0 (QI,...) (r0 dead)
;;         =>  TR <- p1 (QI, ...)
;; -----------------------------------------------
(define_peephole2
  [
  ;;--
  (set (match_operand:QIHISIDI1 0 "gp_register_operand" "")
       (match_operand:QIHISIDI1 1 "pushable_operand" "")
  )
  ;;--
  (set (reg:QIHISIDI2 TR_REGNUM)
       (unspec_volatile:QIHISIDI2 [(reg:QIHISIDI2 TR_REGNUM)
                          (match_operand:QIHISIDI2 2 "register_operand" "")]
         UNSPEC_PUSH_TR)
  )
  ;;--
  ;;--
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOVE_PUSH)
    && (REGNO(operands[0]) == REGNO(operands[2]))
    && peep2_reg_dead_p(2, operands[2])
   )"
  [
  ;;--
  (set (reg:QIHISIDI1 TR_REGNUM)
            (unspec_volatile:QIHISIDI1 [(reg:QIHISIDI1 TR_REGNUM)
                              (match_operand:QIHISIDI1 3 "" "")]
             UNSPEC_PUSH_TR)
  )
  ;;--
  ;;--
  ]
  {
     operands[3] = ivm64_copy_rtx(operands[1]);
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- r1 (r1 dead)
;; TR <- TR << r0 (r0 dead)
;;       => TR <- pow(TR)
;;          TR <- TR * r1
;; -----------------------------------------------
(define_peephole2
  [
  ;;--
  (set (match_operand:DI 0 "gp_register_operand" "")
            (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
  )
  ;;--
  (set (reg:DI TR_REGNUM)
            (unspec_volatile:DI [(reg:DI TR_REGNUM)
                              (match_operand:DI 1 "register_operand" "")]
             UNSPEC_PUSH_TR)
  )
  ;;---
  (set (reg:DI TR_REGNUM)
       (ashift:DI (reg:DI TR_REGNUM) (match_dup:DI 0))
  )
  ;;--
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POW2)
    && peep2_reg_dead_p(2, operands[1])
    && peep2_reg_dead_p(3, operands[0])
   )"
  [
  ;;--
  (set (reg:DI TR_REGNUM)
       (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POW_TR)
  )
  (set (reg:DI TR_REGNUM)
       (mult:DI (reg:DI TR_REGNUM) (match_dup:DI 1))
  )
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- r0 (r dead)
;;       =>  nop
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:ALLMODES 0 "gp_register_operand" "")
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (match_dup:ALLMODES 0)]
           UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_PUSH)
    && peep2_reg_dead_p(2, operands[0]))"
  [
     (const_int 0) ;; nop
  ]
  {

  }
)
;; -----------------------------------------------
;; r0 <- TR (QI, ...)
;; TR <- r0 (r dead)
;;       =>  nop
;; -----------------------------------------------
;;
(define_peephole2
  [
    ;;---
    (set (match_operand:ALLMODES 0 "gp_register_operand" "")
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
      )
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand:DI 1 "register_operand" "")]
               UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_PUSHDI)
    && (REGNO(operands[0]) == REGNO(operands[1]))
    && peep2_reg_dead_p(2, operands[0]))"
  [
     (const_int 0)  ;; nop
  ]
  {
  }
)
;; -----------------------------------------------
;; r <- TR (DI)
;; TR <- r (QI, ...)  (r dead reg)
;;       =>  TR <- TR & const (zero extend)
;;           (const=0xff for QI, 0xffff for HI, 0xffffffff for DI)
;; -----------------------------------------------
;;
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)
                            (match_operand:QIHISI 1 "gp_register_operand" "")]
           UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POPDI_PUSH)
    && (ivm64_extra_peep2_in_progress(3))
    && (REGNO(operands[0]) == REGNO(operands[1]))
    && peep2_reg_dead_p(2, operands[1]))"
  [
    (set (reg:DI TR_REGNUM)
         (and:DI (reg:DI TR_REGNUM) (match_operand:DI 2 "" ""))
    )
  ]
  {
      if (1 == <MODESIZE>)
          operands[2] = GEN_INT(0x0ff);
      else if (2 == <MODESIZE>)
          operands[2] = GEN_INT(0x0ffff);
      else if (4 == <MODESIZE>)
          operands[2] = GEN_INT(0x0ffffffff);
  }
)

;; -----------------------------------------------
;; r0 <- TR (r dead)
;; TR <- mem[r0]
;;       => TR <- mem[TR]
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (mem:ALLMODES (match_dup:DI 0)) ]
           UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_IND_PUSH)
    && peep2_reg_dead_p(2, operands[0])
   )"
  [
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [ (mem:ALLMODES (reg:DI TR_REGNUM)) ]
               UNSPEC_IND_PUSH_TR)
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; r <- TR
;; TR <- mem[r + offset]  (r dead)
;;      => TR <- mem (TR + offset)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (mem:ALLMODES (plus:DI (match_dup:DI 0)
                            (match_operand:DI 1 "immediate_operand")))]
           UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_IND_OFFSET_PUSH)
    && peep2_reg_dead_p(2, operands[0])
   )"
  [
    ( set (reg:ALLMODES TR_REGNUM)
          (unspec_volatile:ALLMODES [
                (mem:ALLMODES (plus:DI (reg:DI TR_REGNUM) (match_dup:DI 1)))]
             UNSPEC_IND_OFFSET_PUSH_TR)
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p1
;; p0 <- TR
;;       =>  p0 <- p1
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (match_operand:ALLMODES 1 "pushable_operand" "")]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (match_operand:ALLMODES 0 "popable_operand" "")
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOVE)
    && !rtx_equal_p(operands[0], operands[1])
   )"
  [
    (set (match_dup:ALLMODES 0) (match_dup:ALLMODES 1))
  ]
  {
  }
)

;; -----------------------------------------------
;; r <- TR
;; nop
;; TR <- r (r dead)
;;       =>  nop
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:ALLMODES 0 "register_operand" "")
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (match_dup:ALLMODES 0)]
           UNSPEC_PUSH_TR)
      )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_NOP_PUSH)
    && peep2_reg_dead_p(2, operands[0])
   )"
  [
     (const_int 0)  ;; nop
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- p1 (QI, ...)
;; TR <- r0 (QI,...) (r0 dead)
;;       =>  TR <- p1
;; -----------------------------------------------
(define_peephole2
  [
    ;--
    (set (match_operand:ALLMODES 0 "gp_register_operand" "")
         (match_operand:ALLMODES 1 "pushable_operand" ""))
    ;--
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (match_dup:ALLMODES 0) ]
           UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOV_PUSH)
    && peep2_reg_dead_p(2, operands[0]))"
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (match_dup:ALLMODES 1) ]
           UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- p1 (QI,...)
;; TR <- r2 (QI,...) (r2 dead)
;; r2 <- TR
;; TR <- r0 (QI,...) (r0 dead)
;; r0 <- TR
;;  =>
;;         r0 <- p1 (QI,...)
;;         TR <- r2 (QI,...)
;;         r2 <- TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:ALLMODES 0 "gp_register_operand" "")
         (match_operand:ALLMODES 1 "pushable_operand" ""))
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                        (match_operand:ALLMODES 3 "gp_register_operand" "") ]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (match_operand:DI 4 "register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                           (match_dup:ALLMODES 0) ]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (match_operand:DI 5 "register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_MOV_PUSH_POP_PUSH_POP)
    && (REGNO(operands[0]) != REGNO(operands[3]))
    && (REGNO(operands[4]) == REGNO(operands[3]))
    && (REGNO(operands[5]) == REGNO(operands[0]))
    && peep2_reg_dead_p(2, operands[3])
    && peep2_reg_dead_p(4, operands[0])
   )"
  [
    ;;---
    (set (match_operand:ALLMODES 0 "gp_register_operand" "")
         (match_operand:ALLMODES 1 "pushable_operand" ""))
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                            (match_operand:ALLMODES 3 "gp_register_operand" "") ]
           UNSPEC_PUSH_TR)
    )
    ;;---
    (set (match_operand:DI 4 "register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; (empty stack)
;; AR <- call(a,b,...) (npopargs = no. of bytes of arguments)
;;                     (no EH region)
;; p4 <- AR (AR dead)
;;      => call()
;;         p4 <- TR
;;         SP <- SP + (npopargs - UNITS_PER_WORD)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (parallel [(set (match_operand:ALLMODES 0 "gp_register_operand" "")
                    (call (match_operand:DI 1 "memory_operand" "")
                             (match_operand:DI 2 "general_operand" "")))
               (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                                         (match_operand:DI 3 "immediate_operand" "")))
              ]
    )
    ;;---
    (set (match_operand:ALLMODES 4 "general_dst_operand" "")
         (match_dup:ALLMODES  0)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_CALL_PUSH_AR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && peep2_reg_dead_p(2, operands[0])
     && (!reg_mentioned_p(operands[0], operands[4]))
     && (INTVAL(operands[3]) >= UNITS_PER_WORD)
     && (!find_reg_note(peep2_next_insn(0), REG_EH_REGION, NULL_RTX)) /* No EH_REGION */
     && (!INSN_ANNULLED_BRANCH_P(peep2_next_insn(0))) /* No flag /u in call */
     && ivm64_reg_args_size_is_zero(peep2_next_insn(0)) /* If some arguments have been already pushed, this peephole is not applicable */
   )"
   [
    ;;---
    (call (match_dup:DI 1)
          (match_dup:DI 2))
    ;;---
    (unspec_volatile
        [(match_operand 5 "const_int_operand" "")] UNSPEC_AR_OFFSET)
    ;;---
    (unspec_volatile [(const_int 100)] UNSPEC_NOP_PUSH_TR)
    (unspec_volatile
        [(match_operand 6 "const_int_operand" "")] UNSPEC_EXTRA_OFFSET)
    ;;---
    (set (match_dup:ALLMODES 4)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    (unspec_volatile
        [(match_operand 7 "const_int_operand" "")] UNSPEC_EXTRA_OFFSET)
    ;;---
    (unspec_volatile
        [(match_operand 8 "const_int_operand" "")] UNSPEC_AR_OFFSET)
    (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                            (match_operand:DI 9 "immediate_operand" "")))
    ;;---
    (use (match_dup:DI 0))
    ;;---
   ]
   {
      long npopargs = INTVAL(operands[3]);
      operands[5] = GEN_INT(-npopargs);

      operands[6] = GEN_INT(npopargs - UNITS_PER_WORD);
      operands[7] = GEN_INT(-npopargs + UNITS_PER_WORD);

      operands[8] = GEN_INT(npopargs - UNITS_PER_WORD);
      operands[9] = GEN_INT(npopargs - UNITS_PER_WORD);

   }
)

;; -----------------------------------------------
;; (empty stack)
;; AR <- call(a,b,...) (npopargs = no. of bytes of arguments)
;;                     (no EH region)
;; mem[SP--] <- AR (AR dead)
;;      => call()
;;         mem[ ] <- ...
;;         SP <- SP + (npopargs - 2*UNITS_PER_WORD)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (parallel [(set (match_operand:ALLMODES 0 "gp_register_operand" "")
                    (call (match_operand:DI 1 "memory_operand" "")
                             (match_operand:DI 2 "general_operand" "")))
               (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                                         (match_operand:DI 3 "immediate_operand" "")))
              ]
    )
    ;;---
    (set (mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
         (match_dup:ALLMODES 0)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_CALL_PUSH_AR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && peep2_reg_dead_p(2, operands[0])
     && (INTVAL(operands[3]) >= 2*UNITS_PER_WORD)
     && (!find_reg_note(peep2_next_insn(0), REG_EH_REGION, NULL_RTX)) /* No EH_REGION */
     && (!INSN_ANNULLED_BRANCH_P(peep2_next_insn(0))) /* No flag /u in call */
     && ivm64_reg_args_size_is_zero(peep2_next_insn(0)) /* If some arguments have been already pushed, this peephole is not applicable */
   )"
   [
    ;;---
    (call (match_dup:DI 1)
          (match_dup:DI 2))
    ;;---
    (unspec_volatile
        [(match_operand 5 "const_int_operand" "")] UNSPEC_AR_OFFSET)
    ;;---
    (unspec_volatile [(const_int 100)] UNSPEC_NOP_PUSH_TR)
    (unspec_volatile
        [(match_operand 6 "const_int_operand" "")] UNSPEC_EXTRA_OFFSET)
    ;;---
    (set (match_dup:ALLMODES 0)
         (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    (unspec_volatile
        [(match_operand 7 "const_int_operand" "")] UNSPEC_EXTRA_OFFSET)
    ;;---
    (unspec_volatile
        [(match_operand 8 "const_int_operand" "")] UNSPEC_AR_OFFSET)
    (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                            (match_operand:DI 9 "immediate_operand" "")))

    ;;---
    (use (match_dup:DI 0))
    ;;---
   ]
   {
      long npopargs = INTVAL(operands[3]);
      operands[5] = GEN_INT(-npopargs);

      operands[6] = GEN_INT(npopargs - 2*UNITS_PER_WORD);
      operands[7] = GEN_INT(-npopargs + 2*UNITS_PER_WORD);

      operands[8] = GEN_INT(npopargs - 1*UNITS_PER_WORD);
      operands[9] = GEN_INT(npopargs - 2*UNITS_PER_WORD);

   }
)

;; -----------------------------------------------
;; (empty stack)
;; AR <- call(a) (only one argument)
;;               (no EH region)
;; mem[SP--] <- AR (AR dead)
;;      => call()
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (parallel [(set (match_operand:ALLMODES 0 "gp_register_operand" "")
                    (call (match_operand:DI 1 "memory_operand" "")
                             (match_operand:DI 2 "general_operand" "")))
               (set (reg:DI SP_REGNUM) (plus:DI (reg:DI SP_REGNUM)
                                         (match_operand:DI 3 "immediate_operand" "")))
              ]
    )
    ;;---
    (set (mem:ALLMODES (pre_dec:DI (reg:DI SP_REGNUM)))
         (match_dup:ALLMODES 0)
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_CALL_PUSH_AR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && peep2_reg_dead_p(2, operands[0])
     && (INTVAL(operands[3]) == UNITS_PER_WORD)
     && (!find_reg_note(peep2_next_insn(0), REG_EH_REGION, NULL_RTX)) /* No EH_REGION */
     && (!INSN_ANNULLED_BRANCH_P(peep2_next_insn(0))) /* No flag /u in call */
     && ivm64_reg_args_size_is_zero(peep2_next_insn(0)) /* If some arguments have been already pushed, this peephole is not applicable */
   )"
   [
    ;;---
    (call (match_dup:DI 1)
          (match_dup:DI 2))
    ;;---
    (use (match_dup:DI 0))
    ;;---
   ]
   {
   }
)

;; -----------------------------------------------
;; set+nop
;; -----------------------------------------------
(define_peephole2
  [
    ;--
    (set (match_operand 0 "" "")
         (match_operand 1 "" ""))
    ;--
    (const_int 0)  ;; nop
    ;;---
  ]
  "ivm64_peep_enabled(IVM64_PEEP2_SET_NOP)
   && ! reg_mentioned_p(pc_rtx, operands[0])
   && ! reg_mentioned_p(pc_rtx, operands[1])
   && (GET_CODE(operands[1]) != CALL)
  "
  [
    ;;---
    (set (match_dup 0) (match_dup 1))
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; nop+set
;; -----------------------------------------------
(define_peephole2
  [
    ;--
     (const_int 0)  ;; nop
    ;--
    (set (match_operand 0 "" "")
         (match_operand 1 "" ""))
    ;;---
  ]
  "ivm64_peep_enabled(IVM64_PEEP2_NOP_SET)
   && ! reg_mentioned_p(pc_rtx, operands[0])
   && ! reg_mentioned_p(pc_rtx, operands[1])
   && (GET_CODE(operands[1]) != CALL)
  "
  [
    ;;---
    (set (match_dup 0) (match_dup 1))
    ;;---
  ]
  {
  }
)

;; -------------------------------------------------------------------------
;; Peepholes for cbranch sequences
;; -------------------------------------------------------------------------

;; -----------------------------------------------
;; r0 <- TR
;; cbranch(r0, p1) (r0 dead)
;;       => cbranch(TR, p1)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:QIHISIDI 0 "register_operand" "")
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (pc)
         (if_then_else (match_operator 2 "ordered_comparison_operator"
                       [   (match_dup:QIHISIDI 0)
                           (match_operand:QIHISIDI 1 "pushable_operand" "")])
                       (label_ref (match_operand 3 "" ""))
                       (pc))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_CBRANCH)
    && ivm64_peep_pop_cmp_p(operands[2], 0)
    && peep2_reg_dead_p(2, operands[0])
    && !reg_mentioned_p(operands[0], operands[1])
  )"
  [
    (set (pc)
         (if_then_else (match_op_dup 2
                       [ (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
                         (match_dup:QIHISIDI 1)])
                       (label_ref (match_dup 3))
                       (pc))
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; cbranch(p1, r0) (r0 dead)
;;       => cbranch(p1, TR)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:QIHISIDI 0 "register_operand" "")
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (pc)
         (if_then_else (match_operator 2 "ordered_comparison_operator"
                       [ (match_operand:QIHISIDI 1 "pushable_operand" "")
                         (match_dup:QIHISIDI 0) ])
                       (label_ref (match_operand 3 "" ""))
                       (pc))
    )
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POP_CBRANCH_REV)
    && ivm64_peep_pop_cmp_p(operands[2], 1)
    && peep2_reg_dead_p(2, operands[0])
    && (REGNO(operands[0]) != REGNO(operands[1]))
    && ! reg_mentioned_p(operands[0], operands[1])
  )"
  [
    (set (pc)
         (if_then_else (match_op_dup 2
                       [ (match_dup:QIHISIDI 1)
                         (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
                         ])
                       (label_ref (match_dup 3))
                       (pc))
    )
  ]
  {
  }
)

(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (pc)
         (if_then_else (match_operator 2 "ordered_comparison_operator"
                       [   (match_operand:QIHISIDI 4 "register_operand" "")
                           (match_operand:QIHISIDI 1 "pushable_operand" "")])
                       (label_ref (match_operand 3 "" ""))
                       (pc))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POPDI_CBRANCH)
    && (REGNO(operands[0]) == REGNO(operands[4]))
    && ivm64_peep_pop_cmp_p(operands[2], 0)
    && peep2_reg_dead_p(2, operands[0])
    && ! reg_mentioned_p(operands[0], operands[1])
  )"
  [
    (set (pc)
         (if_then_else (match_op_dup 2
                       [ (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
                         (match_dup:QIHISIDI 1)])
                       (label_ref (match_dup 3))
                       (pc))
    )
  ]
  {
  }
)

(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (pc)
         (if_then_else (match_operator 2 "ordered_comparison_operator"
                       [   (match_operand:QIHISIDI 1 "pushable_operand" "")
                           (match_operand:QIHISIDI 4 "register_operand" "") ])
                       (label_ref (match_operand 3 "" ""))
                       (pc))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_POPDI_CBRANCH_REV)
    && (REGNO(operands[0]) == REGNO(operands[4]))
    && ivm64_peep_pop_cmp_p(operands[2], 1)
    && peep2_reg_dead_p(2, operands[0])
    && ! reg_mentioned_p(operands[0], operands[1])
  )"
  [
    (set (pc)
         (if_then_else (match_op_dup 2
                       [ (match_dup:QIHISIDI 1)
                         (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR) ])
                       (label_ref (match_dup 3))
                       (pc))
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- p1
;; cbranch(r0, p2) (r0 dead, r0 not in p2)
;;       => cbranch(p1, p2)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:QIHISIDI 0 "register_operand" "")
         (match_operand:QIHISIDI 1 "pushable_operand" "")
    )
    (set (pc)
         (if_then_else (match_operator 3 "ordered_comparison_operator"
                       [   (match_dup:QIHISIDI 0)
                           (match_operand:QIHISIDI 2 "pushable_operand" "")])
                       (label_ref (match_operand 4 "" ""))
                       (pc))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_SET_CBRANCH)
    && peep2_reg_dead_p(2, operands[0])
    && ! reg_mentioned_p(operands[0], operands[2])
  )"
  [
    (set (pc)
         (if_then_else (match_op_dup 3
                       [   (match_dup:QIHISIDI 1)
                           (match_dup:QIHISIDI 2)])
                       (label_ref (match_dup 4))
                       (pc))
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- p1
;; cbranch(p2, r0) (r0 dead, r0 not in p2)
;;       => cbranch(p2, p1)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:QIHISIDI 0 "register_operand" "")
         (match_operand:QIHISIDI 1 "pushable_operand" "")
    )
    (set (pc)
         (if_then_else (match_operator 3 "ordered_comparison_operator"
                       [ (match_operand:QIHISIDI 2 "pushable_operand" "")
                         (match_dup:QIHISIDI 0)])
                       (label_ref (match_operand 4 "" ""))
                       (pc))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_SET_CBRANCH_REV)
    && peep2_reg_dead_p(2, operands[0])
    && ! reg_mentioned_p(operands[0], operands[2])
  )"
  [
    (set (pc)
         (if_then_else (match_op_dup 3
                       [   (match_dup:QIHISIDI 2)
                           (match_dup:QIHISIDI 1)])
                       (label_ref (match_dup 4))
                       (pc))
    )
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- TR binop2 r0 (r0 dead)
;;       =>
;;           TR <- TR binop2 TR
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (match_operand:DI 0 "register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 1 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_dup:DI 0)] )
    )
    ;;---
  ]
  " ivm64_peep_enabled(IVM64_PEEP2_PUSH_BINOP)
    && peep2_reg_dead_p(2, operands[0])
  "
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 1
                      [(unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
                       (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)]
         )
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- r0 (QI, ...)
;; r1 <- TR
;; TR <- r2 (r2 dead)
;; TR <- TR binop r1 (DI) (r1 dead) (r2 != r1)
;;       =>  TR <- r2
;;           TR <- TR binop2 r0 (SI, ...)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                                (match_operand:QIHISIDI 0 "register_operand" "")]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (match_operand:DI 1 "register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand:DI 2 "register_operand" "")]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 3 "binary_not_shift_operator" [(reg:DI TR_REGNUM)
                  (match_dup:DI 1)] )
    )
    ;;---
  ]
  " ivm64_peep_enabled(IVM64_PEEP2_PUSH_POP_PUSH_BINOP_MULTIMODE)
    && REGNO(operands[1]) != REGNO(operands[2])
    && peep2_reg_dead_p(4, operands[1])
    && (ivm64_extra_peep2_in_progress(3))
  "
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 2)]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
         (match_operator:QIHISIDI 4 "binary_operator" [(reg:QIHISIDI TR_REGNUM)
                  (match_dup:QIHISIDI 0)] )
    )
    ;;---
  ]
  {
    operands[4] = operands[3];
    PUT_MODE(operands[4], GET_MODE(operands[0]));
  }
)

;; -----------------------------------------------
;; TR <- mem[TR]  (QI, ....)
;; TR <- zero_extend(TR) (QI, ...)
;; =>
;;     TR <- mem[TR]  (QI, ....)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
         (unspec_volatile:QIHISIDI [
             (mem:QIHISIDI (reg:DI TR_REGNUM))
           ] UNSPEC_IND_PUSH_TR)
    )
    ;;---
    ;;---
    (set (reg:DI TR_REGNUM)
       (zero_extend:DI (reg:QIHISIDI TR_REGNUM))
    )
    ;;---
  ]
  " ivm64_peep_enabled(IVM64_PEEP2_PUSH_INDPUSH_ZERO_EXTEND)
  "
  [
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
         (unspec_volatile:QIHISIDI [
             (mem:QIHISIDI (reg:DI TR_REGNUM))
           ] UNSPEC_IND_PUSH_TR)
    )
    ;;---
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- p0
;; TR <- TR - p1
;; TR <- -TR (neg)
;;       =>
;;          TR <- p1
;;          TR <- TR - p0
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                   (match_operand:DI 0 "arithmetic_operand" "")]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (minus:DI (reg:DI TR_REGNUM) (match_operand:DI 1 "arithmetic_operand" ""))
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_NEG_TR)
    )
  ]
  " ivm64_peep_enabled(IVM64_PEEP2_PUSH_SUB_NEG)
  "
  [
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                   (match_dup:DI 1 )]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (minus:DI (reg:DI TR_REGNUM) (match_dup:DI 0))
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; TR <- TR & 0xffffffff
;; not register operand <- TR (DI)
;;    =>  not regiter operand <- TR (DI) (only low part is stored)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (and:DI (reg:DI TR_REGNUM) (match_operand:DI 0 "const_int_operand" ""))
    )
    ;;---
    (set (match_operand:QIHISI 1 "" "")
              (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  " (ivm64_peep_enabled(IVM64_PEEP2_AND_POP)
     && (((1UL << (8*GET_MODE_SIZE(GET_MODE(operands[1])))) - 1) == (unsigned long)INTVAL(operands[0]))
     && (!REG_P(operands[1]))
  )"
  [
    ;;---
    (set (match_dup:QIHISI 1)
              (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)] UNSPEC_POP_TR)
    )
  ]
  {
  }
)

(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand:DI 0 "register_operand" "")]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (plus: DI (reg:DI TR_REGNUM) (match_operand:DI 1 "arithmetic_operand" ""))
    )
    ;;---
    (set (match_operand:DI 2 "register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_operand:ALLMODES 3 "pushable_operand" "")]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (mem:ALLMODES (match_dup:DI 2))
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  " (ivm64_peep_enabled(IVM64_PEEP2_PUSH_ADD_POP_PUSH_IND_POP)
    && ! reg_mentioned_p(operands[2], operands[3])
    && peep2_reg_dead_p(5, operands[2])
  )"
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_dup:ALLMODES 3)]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 0)]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (plus: DI (reg:DI TR_REGNUM) (match_dup:DI 1))
    )
    ;;---
    (set (mem:ALLMODES (reg:DI TR_REGNUM))
            (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_IND_POP_TR)
    )
  ]
  {
  }
)

;; -----------------------------------------------
;;  TR <- r0
;;  TR <- TR + op1
;;  r2 <- TR
;;  TR <- r3 (QI, ...)
;;  mem[r2 + offset4] <- TR (dead r2) (QI, ...)
;;      =>
;;         TR <- r3
;;         TR <- r0
;;         TR <- TR + op1
;;         TR <- TR + offset4
;;         store (QI, ...)
;; -----------------------------------------------
(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand:DI 0 "register_operand" "")]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (plus: DI (reg:DI TR_REGNUM) (match_operand:DI 1 "arithmetic_operand" ""))
    )
    ;;---
    (set (match_operand:DI 2 "register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_operand:ALLMODES 3 "pushable_operand" "")]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (mem:ALLMODES (plus:DI (match_dup:DI 2)
                                (match_operand:DI 4 "immediate_operand" "")))
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  " (ivm64_peep_enabled(IVM64_PEEP2_PUSH_ADD_POP_PUSH_IND_OFFSET_POP)
    && ! reg_mentioned_p(operands[2], operands[3])
    && peep2_reg_dead_p(5, operands[2])
  )"
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_dup:ALLMODES 3)]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 0)]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (plus: DI (reg:DI TR_REGNUM) (match_dup:DI 1))
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (plus: DI (reg:DI TR_REGNUM) (match_dup:DI 4))
    )
    ;;---
    (set (mem:ALLMODES (reg:DI TR_REGNUM))
            (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)] UNSPEC_IND_POP_TR)
    )
  ]
  {
  }
)

;; -----------------------------------------------
;;  r0  <- r1 (dead r1)
;;  r1  <- r2 (dead r2)
;;  r2  <- r0 (dead r0) (r1 <-> r2)
;;   => TR <- r1
;;      TR <- r2
;;      r1 <- TR
;;      r2 <- TR
;; -----------------------------------------------
(define_peephole2
  [
  ;;--
  (set (match_operand:QIHISIDI 0 "gp_register_operand" "")
         (match_operand:QIHISIDI 1 "gp_register_operand" "")
  )
  ;;--
  (set (match_dup:QIHISIDI 1)
         (match_operand:QIHISIDI 2 "gp_register_operand" "")
  )
  ;;--
  (set (match_dup:QIHISIDI 2)
         (match_dup:QIHISIDI 0)
  )
  ;;--
  ]
  " (ivm64_peep_enabled(IVM64_PEEP2_MOVE_MOVE_MOVE)
    && peep2_reg_dead_p(1, operands[1])
    && peep2_reg_dead_p(2, operands[2])
    && peep2_reg_dead_p(3, operands[0])
    && (REGNO(operands[0]) != REGNO(operands[1]))
    && (REGNO(operands[0]) != REGNO(operands[2]))
    && (REGNO(operands[1]) != REGNO(operands[2]))
  )"
  [
  (set (reg:QIHISIDI TR_REGNUM)
            (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                                 (match_dup:QIHISIDI 1 )] UNSPEC_PUSH_TR)
  )
  ;;---
  (set (reg:QIHISIDI TR_REGNUM)
            (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                                 (match_dup:QIHISIDI 2 )] UNSPEC_PUSH_TR)
  )
  ;;---
  (set (match_dup:QIHISIDI 1)
            (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
  )
  ;;---
  (set (match_dup:QIHISIDI 2)
            (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
  )
  ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- r1 (r1 not depending on r0)
;; TR <- TR binop3 s2 (s2 not depending on r0 )
;; TR <- TR commutative binop4 r0 (r0 dead) (this action involves TR <- r0)
;;       => TR <- r1
;;          TR <- TR binop3 s2
;;          TR <- TR binop4 TR
;;
;; -----------------------------------------------
(define_peephole2
  [
    (set (match_operand:DI 0 "gp_register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                                (match_operand:QIHISIDI 1 "general_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:QIHISIDI2 TR_REGNUM)
         (match_operator:QIHISIDI2 3 "binary_operator" [(reg:QIHISIDI2 TR_REGNUM)
                  (match_operand:QIHISIDI2 2 "arithmetic_operand" "")])
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 4 "aggregation_operator" [(reg:DI TR_REGNUM)
                  (match_dup:DI 0)])
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP2_POP_PUSH_BINOP_COMMUTATIVE)
     && peep2_reg_dead_p(4, operands[0])
     && !(REG_P(operands[1]) && REGNO(operands[1]) == STACK_POINTER_REGNUM)
     && ! reg_mentioned_p(operands[0], operands[1])
     && ! reg_mentioned_p(operands[0], operands[2])
   )"
  [
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                                (match_dup:QIHISIDI 1)]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:QIHISIDI2 TR_REGNUM)
         (match_op_dup:QIHISIDI2 3 [(reg:QIHISIDI2 TR_REGNUM)
                  (match_dup:QIHISIDI2 2 )])
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 4
                [(unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
                 (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)])
    )
    ;;---
  ]
  {
  }
)

;; -----------------------------------------------
;; r0 <- TR
;; TR <- r1 (r1 not depending on r0)
;; TR <- TR binop3 s2 (s2 not depending on r0 )
;; TR <- TR - r0 (r0 dead, this involves TR <- r0)
;;       => TR <- r1
;;          TR <- TR binop3 s2
;;          TR <- TR - TR
;;          TR <- -TR (neg)
;;
;; -----------------------------------------------
(define_peephole2
  [
    (set (match_operand:DI 0 "gp_register_operand" "")
         (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                                (match_operand:QIHISIDI 1 "general_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 3 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 2 "arithmetic_operand" "")])
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 4 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_dup:DI 0)])
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP2_POP_PUSH_BINOP_SUB)
     && peep2_reg_dead_p(4, operands[0])
     && (GET_CODE(operands[4]) == MINUS)
     && !(REG_P(operands[1]) && REGNO(operands[1]) == STACK_POINTER_REGNUM)
     && ! reg_mentioned_p(operands[0], operands[1])
     && ! reg_mentioned_p(operands[0], operands[2])
   )"
  [
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                                (match_dup:QIHISIDI 1)]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 3 [(reg:DI TR_REGNUM)
                  (match_dup:DI 2 )])
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_op_dup:DI 4
                [(unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
                 (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)])
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (neg:DI (reg:DI TR_REGNUM))
    )
    ;;---
  ]
  {
  }
)

;; CASESI multimode -> REQUIRE MODIFYING .MD and output_casesi(() function
(define_peephole2
  [
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)
                                  (match_operand:QIHISI 0 "pushable_operand" "")]
             UNSPEC_PUSH_TR)
     )
    ;;---
    (set (match_operand:DI 1 "gp_register_operand" "")
         (unspec_volatile:DI [ (reg:DI TR_REGNUM) ] UNSPEC_POP_TR)
    )
    ;;---
    (set (pc)
        (if_then_else
          (leu (minus:DI
                    (match_operand 2 "gp_register_operand" "")
                    (match_operand 3 "" ""))
              (match_operand 4 "" ""))
          (mem:DI (plus:DI (label_ref (match_operand 5 "" ""))
                           (mult:DI (minus:DI (match_dup 2)
                                              (match_dup 3))
                                    (const_int 8))))
          (label_ref:DI (match_operand 6 "" ""))))
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP2_PUSH_POP_CASESI)
     && (REGNO(operands[1]) == REGNO(operands[2]))
     && peep2_reg_dead_p(3, operands[2]) )
  "
  [
    ;;---
    (unspec_volatile [(const_int 0)] UNSPEC_BLOCKAGE) ;; nop
    ;;---
    (set (pc)
        (if_then_else
          (leu (minus:DI
                    (match_dup 2)
                    (match_dup 3))
              (match_dup 4))
          (mem:DI (plus:DI (label_ref (match_dup 5))
                           (mult:DI (minus:DI (match_dup 2)
                                              (match_dup 3))
                                    (const_int 8))))
          (label_ref:DI (match_dup 6))))
  ]
  {
   operands[2] = ivm64_copy_rtx(operands[0]);
  }
)
(define_peephole2
  [
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)
                                  (match_operand:QIHISI 0 "pushable_operand" "")]
             UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (match_operand:QIHISI 1 "register_operand" ""))
    )
    ;;---
    (set (match_operand:DI 2 "gp_register_operand" "")
         (unspec_volatile:DI [ (reg:DI TR_REGNUM) ] UNSPEC_POP_TR)
    )
    ;;---
    (set (pc)
        (if_then_else
          (leu (minus:DI
                    (match_operand 3 "gp_register_operand" "")
                    (match_operand 4 "" ""))
              (match_operand 5 "" ""))
          (mem:DI (plus:DI (label_ref (match_operand 6 "" ""))
                           (mult:DI (minus:DI (match_dup 3)
                                              (match_dup 4))
                                    (const_int 8))))
          (label_ref:DI (match_operand 7 "" ""))))
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP2_PUSH_SIGNX_POP_CASESI)
     && (REGNO(operands[1]) == TR_REGNUM)
     && (REGNO(operands[2]) == REGNO(operands[3]))
     && peep2_reg_dead_p(4, operands[3]) )
  "
  [
    ;;---
    (unspec_volatile [(const_int 0)] UNSPEC_BLOCKAGE) ;; nop
    ;;---
    (set (pc)
        (if_then_else
          (leu (minus:DI
                    (match_dup 3)
                    (match_dup 4))
              (match_dup 5))
          (mem:DI (plus:DI (label_ref (match_dup 6))
                           (mult:DI (minus:DI (match_dup 3)
                                              (match_dup 4))
                                    (const_int 8))))
          (label_ref:DI (match_dup 7))))
  ]
  {
   operands[3] = ivm64_copy_rtx(operands[0]);
  }
)

(define_peephole2
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (match_operand:QIHISIDI1 0 "register_operand" ""))
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (match_operand:QIHISIDI2 1 "register_operand" ""))
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP2_SIGNX_SIGNX)
   && REGNO(operands[0]) == TR_REGNUM
   && REGNO(operands[1]) == TR_REGNUM )
   "
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (match_operand 2 "register_operand" ""))
    )
    ;;---
  ]
  {
   enum machine_mode mode = (GET_MODE_SIZE(GET_MODE(operands[0])) < GET_MODE_SIZE(GET_MODE(operands[1])))
                               ?GET_MODE(operands[0]):GET_MODE(operands[1]) ;
   operands[2] = gen_rtx_REG(mode, TR_REGNUM);
  }
)

(define_peephole2
  [
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)
                                  (match_operand:QIHISI 0 "general_operand" "")]
             UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (match_operand 1 "register_operand" ""))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_PUSH_SIGNX)
   && (REGNO(operands[1]) == TR_REGNUM)
   && (GET_MODE_SIZE(GET_MODE(operands[1])) > GET_MODE_SIZE(GET_MODE(operands[0]))))

  "
  [
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)
                            (match_dup:QIHISI 0)]
             UNSPEC_PUSH_TR)
    )
  ]
  {
  }
)

(define_peephole2
  [
    ;;---
    (set (reg:QIHISI1 TR_REGNUM)
         (unspec_volatile:QIHISI1 [(reg:QIHISI1 TR_REGNUM)
                                  (match_operand:QIHISI1 0 "general_operand" "")]
             UNSPEC_PUSH_TR)
     )
    ;;---
    (set (match_operand:DI 1 "gp_register_operand" "")
         (unspec_volatile:DI [ (reg:DI TR_REGNUM) ] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISI2 TR_REGNUM)
         (unspec_volatile:QIHISI2 [(reg:QIHISI2 TR_REGNUM)
                                   (match_operand:QIHISI2 2 "gp_register_operand" "")]
             UNSPEC_PUSH_TR)
    )
    ;;---
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (match_operand:QIHISI2 3 "register_operand" ""))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_PUSH_POP_PUSH_SIGNX)
   && (REGNO(operands[1]) == REGNO(operands[2]))
   && (REGNO(operands[3]) == TR_REGNUM)
   && (GET_MODE_SIZE(GET_MODE(operands[3]))
       > GET_MODE_SIZE(GET_MODE(operands[0]))))
  "
  [
    ;;---
    (set (reg:QIHISI1 TR_REGNUM)
         (unspec_volatile:QIHISI1 [(reg:QIHISI1 TR_REGNUM)
                                   (match_dup:QIHISI1 0)]
             UNSPEC_PUSH_TR)
     )
    ;;---
    (set (match_dup:DI 1)
         (unspec_volatile:DI [ (reg:DI TR_REGNUM) ] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISI2 TR_REGNUM)
         (unspec_volatile:QIHISI2 [(reg:QIHISI2 TR_REGNUM)
                                   (match_dup:QIHISI2 2)]
             UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  {
  }
)

(define_peephole2
  [
    ;;---
    (set (reg:QIHISI1 TR_REGNUM)
         (unspec_volatile:QIHISI1 [(mem:QIHISI1
                                     (match_operand:DI 0 "register_operand" ""))]
             UNSPEC_IND_PUSH_TR)
     )
    ;;---
    (set (match_operand:DI 1 "gp_register_operand" "")
         (unspec_volatile:DI [ (reg:DI TR_REGNUM) ] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISI2 TR_REGNUM)
         (unspec_volatile:QIHISI2 [(reg:QIHISI2 TR_REGNUM)
                                   (match_operand:QIHISI2 2 "gp_register_operand" "")]
             UNSPEC_PUSH_TR)
    )
    ;;---
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (match_operand:QIHISI2 3 "register_operand" ""))
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP2_INDPUSH_POP_PUSH_SIGNX)
   && (REGNO(operands[0]) == TR_REGNUM)
   && (REGNO(operands[3]) == TR_REGNUM)
   && (REGNO(operands[1]) == REGNO(operands[2]))
   && (GET_MODE_SIZE(GET_MODE(operands[3]))
       > GET_MODE_SIZE(GET_MODE(SET_SRC(peep2_next_insn(0))))))
  "
  [
    ;;---
    (set (reg:QIHISI1 TR_REGNUM)
         (unspec_volatile:QIHISI1 [(mem:QIHISI1
                                       (match_dup:DI 0))]
             UNSPEC_IND_PUSH_TR)
     )
    ;;---
    (set (match_dup:DI 1)
         (unspec_volatile:DI [ (reg:DI TR_REGNUM) ] UNSPEC_POP_TR)
    )
    ;;---
    (set (reg:QIHISI2 TR_REGNUM)
         (unspec_volatile:QIHISI2 [(reg:QIHISI2 TR_REGNUM)
                                   (match_dup:QIHISI2 2)]
             UNSPEC_PUSH_TR)
    )
    ;;---
  ]
  {
  }
)

;; -------------------------------------------------------------------------
;; Assembly Peepholes
;; -------------------------------------------------------------------------

(define_peephole
  [
    ;;---
    (set (match_operand:QIHISIDI 0 "register_operand" "")
         (match_operand:QIHISIDI 1 "pushable_operand" "")
    )
    ;;---
    (set (reg:QIHISIDI TR_REGNUM)
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)
                                (match_dup:QIHISIDI 0)]
               UNSPEC_PUSH_TR)
    )
    ;;---
    (set (match_operand:DI 2 "register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "
   ivm64_peep_enabled(IVM64_PEEP1_MOVE_PUSH_POP)
   && (REGNO(operands[0]) == REGNO(operands[2]))
   && (!ivm64_prolog_fast_pop)
  "
   {

      ivm64_output_push(operands[1], <MODE>mode);
      emitted_push_cfun++;

      ivm64_output_pop(operands[0], <MODE>mode);
      emitted_pop_cfun++;

      return "";
   }
)

(define_peephole
  [
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 0 "binary_operator" [(reg:DI TR_REGNUM)
                      (match_operand 1 "const_int_operand" "")]
         )
     )
    ;----
    (set (reg:DI TR_REGNUM)
        (match_operator:DI 2 "binary_operator" [(reg:DI TR_REGNUM)
                (match_operand 3 "const_int_operand" "")]
        )
    )
    ;;---
  ]
  "(ivm64_peep_enabled(IVM64_PEEP1_SHIFTRU63_AND)
    && (GET_CODE(operands[0]) == LSHIFTRT)
    && (GET_CODE(operands[2]) == AND)
    && (INTVAL(operands[1]) == 63)
    && ((INTVAL(operands[3]) & 1) == 1)
  )"
  {
    return "push! %1\;shift_ru";
  }
)

(define_peephole
  [
    ;;---
    (set (match_operand:DI 0 "gp_register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_dup:DI 0)]
               UNSPEC_PUSH_TR)
      )
    ;;---
  ]
  "ivm64_peep_enabled(IVM64_PEEP1_POP_PUSH)
   && (!ivm64_prolog_fast_pop)
  "
  {
    output_asm_insn("load8! &0", NULL);
    ivm64_stack_extra_offset += + UNITS_PER_WORD;
    ivm64_output_pop(operands[0], DImode);
    ivm64_stack_extra_offset += - UNITS_PER_WORD;
    return "";
  }
)

(define_peephole
  [
    ;;---
    (set (match_operand:QIHISIDI 0 "gp_register_operand" "")
              (unspec_volatile:QIHISIDI [(reg:QIHISIDI TR_REGNUM)] UNSPEC_POP_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand:DI 1 "gp_register_operand" "")]
               UNSPEC_PUSH_TR)
      )
    ;;---
  ]
  "ivm64_peep_enabled(IVM64_PEEP1_POP_PUSH)
   && (!ivm64_prolog_fast_pop)
   && (REGNO(operands[0]) == REGNO(operands[1]))
  "
  {
    output_asm_insn("load8! &0", NULL);
    ivm64_stack_extra_offset += + UNITS_PER_WORD;
    ivm64_output_pop(operands[0], DImode);
    ivm64_stack_extra_offset += - UNITS_PER_WORD;
    return "";
  }
)

(define_peephole
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand 0 "gp_register_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 2 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "arithmetic_operand" "")])
    )
    ;;---
    (set (match_dup:DI 0)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSHAR_BINOP_POPAR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && (! reg_mentioned_p(operands[0], operands[1]))
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
  {
   ivm64_output_push(operands[1], DImode);
   fprintf(asm_out_file, "\t%s\n", ivm64_rtxop2insn(operands[2]));
   return "";
  }
)

(define_peephole
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand:DI 0 "pushable_ext_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 2 "aggregation_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "register_operand" "")])
    )
    ;;---
    (set (match_dup:DI 1)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSH_COMMUTATIVE_POPAR)
     && (REGNO(operands[1]) == AR_REGNUM)
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
   {
    ivm64_output_push(operands[0], DImode);
    fprintf(asm_out_file, "\t%s\n", ivm64_rtxop2insn(operands[2]));
    return "";
   }
)

(define_peephole
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand:DI 0 "pushable_ext_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 2 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "register_operand" "")])
    )
    ;;---
    (set (match_dup:DI 1)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSH_SUB_POPAR)
     && (GET_CODE(operands[2]) == MINUS)
     && (REGNO(operands[1]) == AR_REGNUM)
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
   {
    ivm64_output_push(operands[0], DImode);
    fprintf(asm_out_file, "\tsub\n");
    fprintf(asm_out_file, "\tneg\n");
    return "";
   }
)

(define_peephole
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand 0 "register_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 3 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "arithmetic_operand" "")])
    )
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 4 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 2 "arithmetic_operand" "")])
    )
    ;;---
    (set (match_dup:DI 0)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSH_BINOP_BINOP_POPAR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && (! reg_mentioned_p(operands[0], operands[1]))
     && (! reg_mentioned_p(operands[0], operands[2]))
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
   {
    ivm64_output_push(operands[1], DImode);
    fprintf(asm_out_file, "\t%s\n", ivm64_rtxop2insn(operands[3]));
    ivm64_output_push(operands[2], DImode);
    fprintf(asm_out_file, "\t%s\n", ivm64_rtxop2insn(operands[4]));
    return "";
   }
)

(define_peephole
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand 0 "pushable_ext_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 3 "binary_operator" [(reg:DI TR_REGNUM)
                  (match_operand:DI 1 "arithmetic_operand" "")])
    )
    ;;---
    (set (match_operand:DI 2 "gp_register_operand" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSH_BINOP_POPAR)
     && (REGNO(operands[2]) == AR_REGNUM)
     && (REGNO(operands[0]) != AR_REGNUM)
     && (! rtx_equal_p(stack_pointer_rtx, operands[0]))
     && (! reg_mentioned_p(operands[2], operands[0]))
     && (! reg_mentioned_p(operands[2], operands[1]))
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
  {

   ivm64_output_setsp(asm_out_file, 1);
   ivm64_stack_extra_offset += - UNITS_PER_WORD;

   ivm64_output_push(operands[0], DImode);

   ivm64_stack_extra_offset += + UNITS_PER_WORD;
   ivm64_output_push(operands[1], DImode);

   fprintf(asm_out_file, "\t%s\n", ivm64_rtxop2insn(operands[3]));

   return "";
  }
)

(define_peephole
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand 0 "register_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (match_operator:DI 1 "unary_operator" [(reg:DI TR_REGNUM)])
    )
    ;;---
    (set (match_dup:DI 0)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSHAR_UNARY_POPAR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && (GET_CODE(operands[1]) == NOT  || GET_CODE(operands[1]) == NEG)
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
   {
      if (GET_CODE(operands[1]) == NOT){
          return "not";
      }
      if (GET_CODE(operands[1]) == NEG){
          return "neg";
      }
      gcc_unreachable ();
   }
)

(define_peephole
  [
    ;;---
    (set (reg:QIHISI TR_REGNUM)
              (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)
                                (match_operand 0 "register_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (reg:QIHISI TR_REGNUM))
    )
    ;;---
    (set (match_operand:DI 1 "" "")
              (unspec_volatile:DI [(reg:DI TR_REGNUM)] UNSPEC_POP_TR)
    )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSHAR_SIGNX_POPAR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && (REGNO(operands[1]) == AR_REGNUM)
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
   {
      if (<MODE>mode == QImode){
          return "sigx1";
      }
      if (<MODE>mode == HImode){
          return "sigx2";
      }
      if (<MODE>mode == SImode){
          return "sigx4";
      }
      gcc_unreachable ();
   }
)

(define_peephole
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_operand:ALLMODES 1 "general_operand" "")]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (match_operand 0 "register_operand" "")
              (unspec_volatile [(reg TR_REGNUM)] UNSPEC_POP_TR)
      )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSH_POPAR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && (! reg_mentioned_p(operands[0], operands[1]))
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
   {
      ivm64_output_setsp(asm_out_file, 1);
      ivm64_stack_extra_offset += - UNITS_PER_WORD;
      ivm64_output_push(operands[1], <MODE>mode);
      emitted_push_cfun++;
      ivm64_stack_extra_offset += + UNITS_PER_WORD;
      emitted_pop_cfun++;
      return "";
   }
)

(define_peephole
  [
    ;;---
    (set (reg:ALLMODES TR_REGNUM)
              (unspec_volatile:ALLMODES [(reg:ALLMODES TR_REGNUM)
                                (match_operand:ALLMODES 1 "general_operand" "")]
               UNSPEC_PUSH_TR)
      )
    ;;---
    (set (match_operand 0 "register_operand" "")
              (unspec_volatile [(reg TR_REGNUM)] UNSPEC_POP_TR)
      )
    ;;---
  ]
  "( ivm64_peep_enabled(IVM64_PEEP1_PUSH_POPMEMAR)
     && (REGNO(operands[0]) == AR_REGNUM)
     && (GET_CODE(operands[1]) == MEM
         && REG_P(XEXP(operands[1], 0))
         && REGNO(XEXP(operands[1], 0)) == AR_REGNUM
        )
     && (ivm64_stack_extra_offset == 0)
     && (ivm64_gpr_offset == 0)
     && (emitted_push_cfun == emitted_pop_cfun)
   )"
   {
      output_asm_insn("load<MODESIZE>", NULL);
      return "";
   }
)

(define_peephole
  [
    ;;---
    (set (reg:DI TR_REGNUM)
              (unspec_volatile:DI [(reg:DI TR_REGNUM)
                                (match_operand 0 "gp_register_operand" "")]
               UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (plus:DI (reg:DI TR_REGNUM) (match_dup:DI 0))
    )
  ]
  "ivm64_peep_enabled(IVM64_PEEP1_PUSH_ADD)"
   {
    ivm64_output_push(operands[0], DImode);
    emitted_push_cfun++;
    rtx rtx_const2 = GEN_INT(2);
    ivm64_output_push(rtx_const2, DImode);
    fprintf(asm_out_file, "\tmult\n");
    return "";
   }
)

(define_peephole
  [
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI [(reg:QIHISI TR_REGNUM)
                            (match_operand 0 "general_operand" "")]
             UNSPEC_PUSH_TR)
     )
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (reg:QIHISI TR_REGNUM))
    )
    ;;---
  ]
  "ivm64_peep_enabled(IVM64_PEEP1_PUSH_SIGNX)"
  {
   ivm64_output_push(operands[0], <MODE>mode);
   emitted_push_cfun++;

   int modebits = 8 * <MODESIZE>;
   unsigned long x1 = (1UL << (modebits - 1));
   unsigned long x2 = (-1UL << modebits) + x1;
   fprintf(asm_out_file, "\txor! %ld\n", x1);
   fprintf(asm_out_file, "\tadd! %ld\n", x2);

   return "";
  }
)

(define_peephole
  [
    ;;---
    (set (reg:QIHISI TR_REGNUM)
         (unspec_volatile:QIHISI
               [(mem:QIHISI (match_operand:DI 0 "tr_register_operand" ""))]
            UNSPEC_IND_PUSH_TR)
    )
    ;;---
    (set (reg:DI TR_REGNUM)
         (sign_extend:DI (reg:QIHISI TR_REGNUM))
    )
    ;;---
  ]
  "ivm64_peep_enabled(IVM64_PEEP1_IND_PUSH_SIGNX)"
  {
   fprintf(asm_out_file, "\tload<MODESIZE>\n");
   int modebits = 8 * <MODESIZE>;
   unsigned long x1 = (1UL << (modebits - 1));
   unsigned long x2 = (-1UL << modebits) + x1;
   fprintf(asm_out_file, "\txor! %ld\n", x1);
   fprintf(asm_out_file, "\tadd! %ld\n", x2);
   return "";
  }
)
