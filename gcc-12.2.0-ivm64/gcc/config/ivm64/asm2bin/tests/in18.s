# Testing complex stack expressions
start:
    push! 2
    push! 1

s1:
    load8! &(load8 (+ -8 &$(+ &0 -&0)))
    load8! &$0
    load1! &$$0
    push!  $$$$$$$$$0

s2: # These two should push zero
    push! (+ (/s (+ &(+ 0 &0) -&0) 8) -&0)
    push! (+ (/s (+ &&0 -&0) 8) -&0)

end:
exit
