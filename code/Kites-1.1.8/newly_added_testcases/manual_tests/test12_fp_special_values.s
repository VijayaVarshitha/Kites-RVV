# Test 12: floating-point special values
li t0, 4
vsetvli x0, t0, e32, m1, tu, mu
li t1, 0x7f800000         # +infinity
fmv.w.x ft0, t1
li t2, 0x7f7fffff         # largest float
fmv.w.x ft1, t2
fmv.w.x ft2, x0           # 0.0
vfmv.v.f v1, ft0          # inf
vfsub.vv v2, v1, v1       # inf - inf = NaN (0x7fc00000)
vfmul.vf v3, v1, ft2      # inf * 0 = NaN (0x7fc00000)
vfmv.v.f v4, ft1
vfadd.vv v5, v4, v4       # max + max = overflow -> inf (0x7f800000)
