# Testing constant expressions operations

jump! main # nop

main:

#push! (* (| 123 2400) (^ 10000 10000) )  # fails with 'ivm as': this expression should be 0 !!!
#push! (* 123 (^ 10000 10000 ))           # fails with 'ivm as': this expression should be 0 !!!

rem_u!! (/u (^ (%u 1234567890123456 0x462d53c8abac) (/u 9421842194218421 (& 1 0xffffffffffffffff) )) 999999989) 0x999233
push! (& (+ 509 (* (* 123 2400)  (^ 10001 10000) )) (^ 8976543210 0xaabbcc))
push! (+ (& 619 (& (| 2401 2401) (& 10001 10000) )) (^ 8976543210 0xfedcba))
add
push! (& (+ 719 (| (& 2401 2401) (| 10001 10000) )) (^ 8976543210 0xabcdef))
mult
push! (/u 0xffff0000ffff0000 0xcafedecada1990)
div_u
rem_u
push! (^ 0xfffffffffff ~(+  ~(+ 1 -0) -(* 112 213)))
push! -(^ 0 (+  ~(+ 1 0) -(+ -~~1 (* 21 24))))
rem_u
rem_u

push! (>>u (* -1 (<< 2 4)) (+ (<< 1 2) (<< 1 0)))
push! (/u (<< 12345 61) (<< 33 32))
rem_u

xor
not
not
not
not

push! (+ 0x8000000000000000 -(+ 0 0x8000000000000000)) # This should be 0
push! (+ 0x8000000000000000 (+ 0 -0x8000000000000000)) # This should be 0
or
add

divide_by_zero:
push! (| (| (/u 12345 0) (/u -12345 0)) (/u 12345 -0))
push! (| (| (/u 0 0) (/u 0 -0)) (| (/u -0 0) (/u 0 0)))
or
push! (| (| (/s 12345 0) (/s -12345 0)) (/s 12345 -0))
or
push! (| (| (/s 0 0) (/s 0 -0)) (| (/s -0 0) (/s 0 0)))
or
push! (| (| (%u 12345 0) (%u -12345 0)) (%u 12345 -0))
or
push! (| (| (%u 0 0) (%u 0 -0)) (| (%u -0 0) (%u 0 0)))
or
push! (| (| (%s 12345 0) (%s -12345 0)) (%s 12345 -0))
or
push! (| (| (%s 0 0) (%s 0 -0)) (| (%s -0 0) (%s 0 0)))
or

bye:
exit
end:
