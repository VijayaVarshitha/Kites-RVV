# Test 10: vfadd / vfsub / vfmul (.vv and .vf), single precision
li t0, 4
vsetvli x0, t0, e32, m1, tu, mu
li t1, 0x3fc00000         # 1.5
fmv.w.x ft0, t1
li t2, 0x40000000         # 2.0
fmv.w.x ft1, t2
vfmv.v.f v1, ft0          # 1.5
vfmv.v.f v2, ft1          # 2.0
vfadd.vv v3, v1, v2       # 3.5
vfsub.vv v4, v1, v2       # -0.5
vfmul.vv v5, v1, v2       # 3.0
vfadd.vf v6, v1, ft1      # 3.5
vfsub.vf v7, v1, ft1      # -0.5
vfmul.vf v8, v1, ft0      # 2.25
