# Checking numeric alias

a=0xa
b=0xab
c=0xaabbccdd
d=0xaabbccdd00000000


ma=-0xa
mb=-0xab
mc=-0xaabbccdd
md=-0xaabbccdd00000000

push *[a b c d 0 z]
push *[ma mb mc md 0 z]

z = 0

