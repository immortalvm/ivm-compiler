
start:

    alias0 = alias1
    alias1 = alias2
    alias2 = alias3

    load1! start
    load1! alias1
    load1! alias2
    load1! alias3
    load1! alias4

    push!   ( + (+(+ -alias4 alias3) (+ alias2 -alias1)) (+ alias4 -start))

    alias3 = alias4
    alias4 = start

operand_lists:
    push!!! 888 777 -2
    push *[(+ $0 (load8 &1) ) (+ (load8 &0) (load8 &0))]
    push *[$0 $0 $0 $1 $3 $2 $4 $0]

numeric_aliases:
    pi = 3
    push!! pi -pi
    push!! pi ~pi
    push * [~pi -pi]

mixing_numbers_and_letters:
    X123=0x321
    push! (+ 0X123 X123)
    push!! 000X123 # should be equivalent to push!! 000 X123

    xaB12=0xa12
    push!! 000xaB12

    push! (+ -10xaB12) # should be equivalent to (+ -10 xaB12)


bye: exit



