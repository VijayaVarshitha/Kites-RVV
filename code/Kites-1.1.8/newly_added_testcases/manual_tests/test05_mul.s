# Test 5: vmul (.vv and .vx), result keeps low SEW bits
li t0, 8
vsetvli x0, t0, e16, m1, tu, mu
li t1, 300
vmv.v.x v1, t1            # 300 in every 16-bit element
vmul.vv v2, v1, v1        # 300*300 = 90000 -> low 16 bits = 24464 (0x5f90)
li t2, -2
vmul.vx v3, v1, t2        # -600 = 0xfda8
