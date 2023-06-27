# Complex stack addressing

push! -1

a=0
load8! &a
push! $a

push! $(load8 pos)

exit

pos: data8 [2]
