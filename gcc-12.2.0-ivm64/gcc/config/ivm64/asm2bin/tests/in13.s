#Test mnemonic as labels in alias

    load8! (load8 space)
    load8! EXPORT
    push! 0

.ddd1:
    jump_zero! jump
    jump_zero! add
    jump_zero! jzfwd

    push! 33

jzfwd:
jump_zero:

    push! (+ (* set_pc getpc) push)
    push=-44
    set_pc= 22
    getpc = 13

exit

jump=jumpy
jumpy=jump_zero
add = jzfwd

EXPORT EXPORT
EXPORT: data8 [-7]
EXPORT space
space: space 10
