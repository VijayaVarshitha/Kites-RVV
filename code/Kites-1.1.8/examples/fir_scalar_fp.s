# FIR filter, SCALAR FLOATING-POINT version (RV64F)
# out[n] = c0*in[n] + c1*in[n+1] + c2*in[n+2] + c3*in[n+3],  n = 0..31   (single precision)
# Expected output: out[n] = 4.25*n + 13  ->  13.0, 17.25, 21.5, ... 144.75
# Compare with fir_vector_fp.s: same data, same result, different instruction count.
.data
input:  .float 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0, 33.0, 34.0, 35.0
coeffs: .float 0.5, 0.25, 2.0, 1.5
output: .zero 128

.text
    la   a0, input          # a0 = &in[n]
    la   a1, coeffs         # a1 = &c[0]
    la   a2, output         # a2 = &out[n]
    li   t0, 32             # outputs remaining

outer:
    fmv.w.x ft0, x0         # acc = 0.0
    mv   t2, a0             # input pointer for this output
    mv   t3, a1             # coefficient pointer
    li   t4, 4              # taps remaining
inner:
    flw  ft1, 0(t3)         # c[k]
    flw  ft2, 0(t2)         # in[n+k]
    addi t3, t3, 4          # (one instruction between load and use: no load-use stall)
    fmadd.s ft0, ft1, ft2, ft0   # acc += c[k]*in[n+k]
    addi t2, t2, 4
    addi t4, t4, -1
    bnez t4, inner

    fsw  ft0, 0(a2)         # out[n] = acc
    addi a0, a0, 4
    addi a2, a2, 4
    addi t0, t0, -1
    bnez t0, outer
