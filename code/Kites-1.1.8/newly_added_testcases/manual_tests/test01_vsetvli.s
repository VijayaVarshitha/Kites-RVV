# Test 1: vsetvli sets VL from AVL, SEW (VLEN = 128)
li t0, 4
vsetvli t1, t0, e32, m1, tu, mu
li t0, 7
vsetvli t2, t0, e32, m1, tu, mu
li t0, 3
vsetvli t3, t0, e64, m1, tu, mu
li t0, 20
vsetvli t4, t0, e8, m1, tu, mu
li t0, 40
vsetvli t5, t0, e8, m1, tu, mu
vsetvli t6, x0, e16, m1, tu, mu
