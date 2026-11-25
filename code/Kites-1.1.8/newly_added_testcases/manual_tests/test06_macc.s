# Test 6: vmacc (multiply-accumulate: vd = vs1*vs2 + vd)
li t0, 4
vsetvli x0, t0, e32, m1, tu, mu
li t1, 2
vmv.v.x v1, t1            # 2
li t2, 7
vmv.v.x v2, t2            # 7
li t3, 1
vmv.v.x v3, t3            # acc = 1
vmacc.vv v3, v1, v2       # acc = 2*7 + 1 = 15
li t4, 10
vmacc.vx v3, t4, v2       # acc = 10*7 + 15 = 85
