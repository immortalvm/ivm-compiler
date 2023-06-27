start:

    load8! (load8 (load8 (load8 ind0) ))
    store8! a
    load8! (+ (load8 (load8 (load8 ind0) )) 8)
ab:
    store8! b
    load1! a
    load1! b

xyz:
    store8!! 1001 z
    and!! (* ~(load4 z) -(load4 y)) -(* -(load4 z) ~(load4 y))

set_sp:

    store8! &0 # equivalent to set_sp &1
    set_sp! &1

push!! 0 0

end:
    exit


ind0: data8[ind1 ]
ind1:
    data8[ ind2]
ind2:
    data8[ ind3     ]
ind3:
    data8[ -1 -2 -3]

bytes:
a: data2 [0]
b: data2 [0]

halfwords:
x: data4 [-3]
y: data4 [0xffffaabb]
z: data4 [0]*5





