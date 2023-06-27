# Testing unicode

Ça_va_garçon = 100

    eñe :
    push! 13
    load8! &0
    jump_not_zero!  ø

güeb:
    push! 18

ø = naïve
naïve = Äällô_òlé

Äällô_òlé

:
    push! 12
    push! oh_là_là_gâteau

    load1! Ç
    load2! ÇÇ
    load4! ÇÇÇÇ

    exit

oh_là_là_gâteau = Ça_va_garçon

ñ: data1[ 80 ]
ññ: data1[ (+ ññ -Ç) (+ -ññ Ç) ]
ññññ: data4 [ (+ ññññ -ÇÇ) ]

Ç=ñ
ÇÇ=ññ
ÇÇÇÇ=ññññ
