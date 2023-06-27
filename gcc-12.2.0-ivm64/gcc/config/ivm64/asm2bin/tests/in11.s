# Testing complex expressions in multiple sized data

load1! data2
load1! (+ 1 data2)
load1! (+ 2 data2)
load1! (+ 3 data2)
load1! (+ 4 data2)
load1! (+ 5 data2)
load1! (+ 6 data2)
load1! (+ 7 data2)
load1! (+ 8 data2)
load1! (+ 9 data2)
load1! (+ 10 data2)
load1! (+ 11 data2)

exit

data2:
data2 [ (+ 13 (+ -data2 --~~data2))    -1]  # 2*2

data1:
data1 [ (+ -3 (+ data1 -~~data1))     145]  # 2*1

data4:
data4 [ (& (+ data4 -data2) -1)          ]  # 4*1

data1_2:
data1 [ (+ -127 (+ -data1 data1))      -3]  # 2*1

