# Checking shift_rs (zero, one, two operands)

push! 63
pow2

s1:
    shift_rs! 63

s2:
    push! (<< 1 63)
    push! 52
    shift_rs

s2b:
    push! 3
    push! 4
    push! (<< 5 13)
    shift_rs! $2
    shift_rs! (+ 1 $2)

s3: load8! op2
    load8! op1
    shift_rs! (+  (load8 op2)(+ 2 $1))
    store8! &1 # remove &1

s4:
    push! 0
    shift_rs!! (+ (load1 op2) (load8 op1)) (+ (* $0 $0) (load8 op2))
    store8! &1 # remove &1

s5:
    push! (>>s (load8 op3) (+ -5 (<< 1 (load2 op4))))

s6:
    push!  1
    load8! op1
    shift_rs! (+ (>>s -10 (<< $1 $1)) (<< 1 (load2 op4)))
    store8! &1 # remove &1

s7:
    shift_rs!! (load8 op5) ( + (+ (>>s -10 (<< $1 $1)) (<< 1 (load2 op4))) (+ ~5 (+ $0 -$0)))

s8:
    load8! op5
    load2! op4
    push! 2
    push! 3
    shift_rs!! $$0 (* $$1 $(load8 &(+ $1 -1)))
    shift_rs!  (+ (<< $2 1) $(load8 (+ 8 &0)))
    store8! &1 # remove &1
    store8! &1
    store8! &1

bye:
exit

op1: data8 [0x8000000000000000]
op2: data8 [ 0 ]

op3: data8 [0x7000000000000000]
op4: data2 [ 6 ]

op5: data8 [0xbe0000000000000f]
