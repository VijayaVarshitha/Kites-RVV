# Test 11: vfmacc (fused multiply-add: vd = vs1*vs2 + vd), vfmv.v.f
li t0, 4
vsetvli x0, t0, e32, m1, tu, mu
li t1, 0x40400000         # 3.0
fmv.w.x ft0, t1
li t2, 0x3f800000         # 1.0
fmv.w.x ft1, t2
vfmv.v.f v1, ft0          # 3.0
vfmv.v.f v2, ft1          # acc = 1.0
vfmacc.vv v2, v1, v1      # 3*3 + 1 = 10.0
vfmacc.vf v2, ft0, v1     # 3*3 + 10 = 19.0
