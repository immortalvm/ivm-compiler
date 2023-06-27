start:

jump! prog

    a: space 1

    v: data8 [0]*3

    w = -1
    x = -255
    y = 2022
    z = 20212022

prog:
    store8!! 25 (load8 a)
    store8!! -1 (load8 b)
    store8!! (sigx1 0xff) (load8 c)

    push!!! (load8 (load8 a)) (load8 (load8 b)) (load8 (load8 c))
    push!!! (load1 (load8 a)) (load1 (load8 b)) (load1 (load8 c))

    store8!! (load8 (load8 a)) v
    store8!! -(load8 (load8 b)) (+ 8 v)
    store8!! (load8 (load8 c)) (+ 16 v)

binary:
    push!!  (>=u (>>s w 63) (/s y -y)) (* (%s z x) (%u z y))

shl:
    push!  (<< w z) # Wrong in ivm, ok in as0-as1?

shr:
    push!!! (>>s w y) (>>u w y) (>>s -y 63)

exit

    b: space 2
    c: space 20
