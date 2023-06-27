start:

    push! 3
    push !  -33

    push!! 200 300

    push! 0xff

manypush:
    push!!!! push push 32 push

    push! &0
    push!!! &1 2 &-99

    push! 0xf0000001
    push! 0xf0000000000001

push:
push!!!!! &23 &24 25 &26 &27

dollar:
    push!!! $10 $13 $-14

load4:
load8! load4

push!! 43 &-1


nooper:
load4
push

final:
    data8 [0] # = exit
exit
