start:
    push!!!!! -1 -2 -3 -4 -5
    push! $3

    push!! -$4 ~$1
    push! &1
    load4

    push! (+ one -two)

complex:
    load8! -(+ 0 -(+ (+ data 0) -(+ -two one)))

and:
  # x&~x should be 0
  and!! and ~and

add:
  # x+~x should be -1
  add!! ~add add

multipush:
  push* [11 22 33 44 ]

push_mnemo:
    push* [ (+ -one data8) (+ export -two) (+ data8 -export) ]

theend:
exit

data:
set_sp:
setsp:
space:
data8:
    data8 [ -3
            ~(+ (+ -setsp 200) set_sp)
            -(+ (+ 0 -two) -(+ -one -3))
     start data8]

one:
data8 [1]

two:
export:
data8 [2]
