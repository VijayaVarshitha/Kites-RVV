# FIR filter, SCALAR version (RV64IM)
# out[n] = c0*in[n] + c1*in[n+1] + c2*in[n+2] + c3*in[n+3],  n = 0..31
# Expected output: [51, 68, 85, 102] ... 578
# Compare with fir_vector.s: same data, same result, different instruction count.
.data
input:  .word 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
coeffs: .word 2, 3, 5, 7
output: .zero 128

.text
    la   a0, input          # a0 = &in[n]
    la   a1, coeffs         # a1 = &c[0]
    la   a2, output         # a2 = &out[n]
    li   t0, 32             # outputs remaining

outer:
    li   t1, 0              # acc = 0
    mv   t2, a0             # input pointer for this output
    mv   t3, a1             # coefficient pointer
    li   t4, 4              # taps remaining
inner:
    lw   t6, 0(t3)          # c[k]
    lw   t5, 0(t2)          # in[n+k]
    addi t3, t3, 4          # (one instruction between load and use: no load-use stall)
    mul  t5, t5, t6
    add  t1, t1, t5         # acc += c[k]*in[n+k]   (uses mul result immediately)
    addi t2, t2, 4
    addi t4, t4, -1
    bnez t4, inner          # (uses addi result immediately)

    sw   t1, 0(a2)          # out[n] = acc
    addi a0, a0, 4
    addi a2, a2, 4
    addi t0, t0, -1
    bnez t0, outer
