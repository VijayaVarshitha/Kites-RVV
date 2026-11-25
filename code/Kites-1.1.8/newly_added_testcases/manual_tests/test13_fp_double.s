# Test 13: double precision (e64)
li t0, 2
vsetvli x0, t0, e64, m1, tu, mu
li t1, 0x3ff8000000000000   # 1.5
fmv.d.x ft0, t1
li t2, 0x4004000000000000   # 2.5
fmv.d.x ft1, t2
vfmv.v.f v1, ft0            # 1.5, 1.5
vfadd.vf v2, v1, ft1        # 4.0  = 0x4010000000000000
vfmul.vf v3, v2, ft1        # 10.0 = 0x4024000000000000
