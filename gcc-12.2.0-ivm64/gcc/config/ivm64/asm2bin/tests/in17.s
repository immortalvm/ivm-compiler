# Checking jump (zero, one, two operands)

j1:
    push! after1   # equivalent to call
    jump! (load8 op1)
after1:

c1:
    call! exit1
    call! (load8 op1)

    load8! op1
    call! (load8 (* (+ (+ ~8 1) &1) (+ 1 (+ -$0 $0 ))))
    store8! &1

j2:
    load8! op2
    jump! (+ (+ -$0 (load8 op2)) $0)
after2:
    store8! &1

j3:
    load8! op3
    push! (+ -8 exit3)
    jump_zero!! (+ ~~(load8 (+ -8 &2)) -$1) (+ 8 $0)
after3:
    store8! &1
    store8! &1

end:
    push! 111
    exit
#----------------

pad: data1 [0xe7]*13

#----------------
exit1:
    call! inc2
    push! $0
    store8!! (+ -51 (load2 c)) &1
    return

exit2:
    call! inc2
    load2! c
    jump! after2

exit3:
    call! inc2
    load2! c
    jump_not_zero!! $0 after3

push! -1 # Never reached
exit

c: data2[100] # A counter

op1: data8 [exit1]
op2: data8 [exit2]
op3: data8 [exit3]

inc2: # increment c twice
    add!! (load2 c) 1 #inc c
    store2! c
    store2!! (+ (load2 c) 1) c # inc c
    return
