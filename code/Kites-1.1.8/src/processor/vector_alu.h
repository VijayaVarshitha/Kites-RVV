/**
 * @file vector_alu.h
 * @brief Decode and lane arithmetic for the supported RVV integer instructions.
 *
 * Shared by the single-cycle processor and the five-stage pipelines so every
 * processor model computes vector results identically.
 *
 * Supported (unmasked, vm = 1):
 *   vadd.vv  vadd.vx   funct6 000000   OPIVV / OPIVX
 *   vsub.vv  vsub.vx   funct6 000010   OPIVV / OPIVX
 *   vmul.vv  vmul.vx   funct6 100101   OPMVV / OPMVX
 *   vmacc.vv vmacc.vx  funct6 101101   OPMVV / OPMVX   vd[i] = vs1|rs1 * vs2[i] + vd[i]
 *   vmv.v.v  vmv.v.x  vmv.v.i
 *                      funct6 010111   OPIVV / OPIVX / OPIVI, vs2 = 0
 *                                      vd[i] = vs1[i] | rs1 | sign-extended simm5
 */
#ifndef VECTOR_ALU_H
#define VECTOR_ALU_H

#include <cstdint>

namespace Kites::vector_alu
{

enum class Op
{
    None,
    AddVV,
    AddVX,
    SubVV,
    SubVX,
    MulVV,
    MulVX,
    MaccVV,
    MaccVX,
    MvVX,
    MvVV,
    MvVI
};

constexpr uint8_t OPIVV = 0b000;
constexpr uint8_t OPMVV = 0b010;
constexpr uint8_t OPIVI = 0b011;
constexpr uint8_t OPIVX = 0b100;
constexpr uint8_t OPMVX = 0b110;

/// Decodes an OP-V (opcode 1010111) arithmetic instruction. Returns Op::None if unsupported.
inline Op Decode(uint32_t instruction)
{
    const uint8_t funct3 = (instruction >> 12U) & 0x7U;
    const uint8_t vs2 = (instruction >> 20U) & 0x1FU;
    const uint8_t vm = (instruction >> 25U) & 1U;
    const uint8_t funct6 = (instruction >> 26U) & 0x3FU;
    if ((instruction & 0x7FU) != 0b1010111 || vm != 1)
        return Op::None;
    switch (funct6)
    {
    case 0b000000:
        return funct3 == OPIVV ? Op::AddVV : funct3 == OPIVX ? Op::AddVX : Op::None;
    case 0b000010:
        return funct3 == OPIVV ? Op::SubVV : funct3 == OPIVX ? Op::SubVX : Op::None;
    case 0b100101:
        return funct3 == OPMVV ? Op::MulVV : funct3 == OPMVX ? Op::MulVX : Op::None;
    case 0b101101:
        return funct3 == OPMVV ? Op::MaccVV : funct3 == OPMVX ? Op::MaccVX : Op::None;
    case 0b010111:
        if (vs2 != 0)
            return Op::None;
        return funct3 == OPIVX ? Op::MvVX : funct3 == OPIVV ? Op::MvVV : funct3 == OPIVI ? Op::MvVI : Op::None;
    default:
        return Op::None;
    }
}

/// True if bits 19:15 name a vector register (vs1); false if they name a scalar x register.
inline bool UsesVectorSource1(Op op)
{
    return op == Op::AddVV || op == Op::SubVV || op == Op::MulVV || op == Op::MaccVV || op == Op::MvVV;
}

/// True if bits 19:15 hold a 5-bit signed immediate (vmv.v.i) instead of a register.
inline bool UsesImmediate(Op op) { return op == Op::MvVI; }

/// The sign-extended simm5 in bits 19:15.
inline uint64_t Immediate(uint32_t instruction)
{
    const uint64_t simm5 = (instruction >> 15U) & 0x1FU;
    return (simm5 & 0x10U) ? (simm5 | ~uint64_t{0x1F}) : simm5;
}

/// True if the old value of vd is an input (multiply-accumulate).
inline bool ReadsDestination(Op op) { return op == Op::MaccVV || op == Op::MaccVX; }

/// One lane: a = vs2[i], b = vs1[i] or the scalar, d = old vd[i]. Result is masked by the caller.
inline uint64_t Compute(Op op, uint64_t a, uint64_t b, uint64_t d)
{
    switch (op)
    {
    case Op::AddVV:
    case Op::AddVX:
        return a + b;
    case Op::SubVV:
    case Op::SubVX:
        return a - b;
    case Op::MulVV:
    case Op::MulVX:
        return a * b;
    case Op::MaccVV:
    case Op::MaccVX:
        return b * a + d;
    case Op::MvVX:
    case Op::MvVV:
    case Op::MvVI:
        return b;
    default:
        return d;
    }
}

} // namespace Kites::vector_alu

#endif // VECTOR_ALU_H
