# Test 8: tail elements stay unchanged when VL < VLMAX
li t0, 4
vsetvli x0, t0, e32, m1, tu, mu
vmv.v.i v1, 9             # 9,9,9,9
li t0, 2
vsetvli t1, t0, e32, m1, tu, mu   # VL = 2
vmv.v.i v1, 1             # only elements 0,1 change -> 1,1,9,9
vadd.vv v1, v1, v1        # -> 2,2,9,9
