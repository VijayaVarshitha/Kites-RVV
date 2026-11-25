# Test 7: vmv.v.v, vmv.v.x, vmv.v.i
li t0, 4
vsetvli x0, t0, e32, m1, tu, mu
li t1, 42
vmv.v.x v1, t1            # 42
vmv.v.v v2, v1            # copy: 42
vmv.v.i v3, -1            # 0xffffffff
vmv.v.i v4, 15            # 15
vmv.v.i v5, -16           # 0xfffffff0
