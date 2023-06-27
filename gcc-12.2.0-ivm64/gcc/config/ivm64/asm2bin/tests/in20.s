# testing 'space' directive with complex expressions

start:

push! ( + (load8 s2) -(load8 s1))
push! ( + (load8 s3) -(load8 s1))
push! ( + (load8 s4) -(load8 s1))
push! ( + (load8 s5) -(load8 s1))
push! ( + (load8 s6) -(load8 s1))
push! ( + (load8 s7) -(load8 s1))

e:
exit

s1: space 1
s2: space 100

pad1: data1 [0] * 417  # This pad is in the program area

s3: space (+ 0 -~~-1001)
s4: space (+ s7 -(+ s4 (+ s3 -s1)))

pad2: data1 [0] * 113

s5: space (+ (+ &4 -&2) (+ -&-10 &-4))
s6: space (+ (+ s7 -s5) (+ 50 50))
s7: space 1
