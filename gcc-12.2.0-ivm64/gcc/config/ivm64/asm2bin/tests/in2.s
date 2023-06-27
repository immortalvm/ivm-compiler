push!!!!!!! 100 101 102 103 104 105 107
start:
    push!!! $2 $3 $5

start2:
    push! (+ (+ 12 (+ $3 $2)) (+ (+ $1 -13) $5) )

push! -start2

exit

data:
data4:
data8:
data8 [ 33 data8 44 start2 data8 (+ -data4 -(+ 1  -start2))]
