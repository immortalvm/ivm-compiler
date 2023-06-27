# Complex stack addressing and multiple negations
start:

operand_lists:
    push!!!! -123 888 777 -2

    push!!!!!! $0 $1 $2 $2 $1 $0
    push *[$0 $1 $2 $2 ~$1 -$0]

    push *[$m0 $m1 $m2 $m2 ~$m3 -$m4]

    push *[(+ -$0 -(load8 &5) ) (+ (load8 &0) (load8 &0))]
    push *[~(+ -$m0 -(load8 &5) ) -(+ ~(load8 &m0) -(load8 &m1))]

    load4! data
    load4! (+ 4 data)
    load2! (+ 8 data)

    push! $(+ (load4 data) 4)
    push! (+ ~-~(load4 data) ~-~-(* 2 ~(load4 data)))

    push! -~--~~-~0x10
    push! ~-~--~$-~--~~-~0x10

    push! $(+ 0 0)

bye: exit

z=0
u=1
t=2

data:
    data4 [ -~(+ ~--~(+ -3 4) -------------2)  -(+ data ---data2)]

data2:
    data2 [ ~(+ (+ (+ bye -bye) bye) -data2) ]

m0=z
m1=u
m2=t
m3=3
m4=4


