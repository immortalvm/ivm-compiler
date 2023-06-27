# Testing jumps
push! -1
push! bye

jump_zero!  ( + (+ ---jump jump) (+ (+ $~~0 -1) 1) )

    push! 22
    push! jump

jump_not_zero! (+ (+ $~~0 -22) (+ (+ ---bye jump) 22))

    push! 33

jump:
bye:

    push! 0
    jump_zero! exit

exit:
    put_char! 66
    put_char! 121
    put_char! 101
    put_char! 10
    exit

