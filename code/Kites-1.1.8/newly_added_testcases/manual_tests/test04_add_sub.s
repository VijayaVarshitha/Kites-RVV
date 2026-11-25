# Test 4: vadd / vsub (.vv and .vx)
li t0, 4
vsetvli x0, t0, e32, m1, tu, mu
li t1, 5
vmv.v.x v1, t1            # v1 = 5,5,5,5
li t2, 3
vmv.v.x v2, t2            # v2 = 3,3,3,3
vadd.vv v3, v1, v2        # 8
vsub.vv v4, v2, v1        # 3-5 = -2
li t3, 100
vadd.vx v5, v1, t3        # 105
vsub.vx v6, v1, t3        # 5-100 = -95
