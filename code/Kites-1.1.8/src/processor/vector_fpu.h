/**
 * @file vector_fpu.h
 * @brief Decode and lane arithmetic for the supported RVV floating-point instructions.
 *
 * Shared by the single-cycle processor and the five-stage pipelines so every
 * processor model computes vector FP results identically.
 *
 * Supported (unmasked, vm = 1, SEW = 32 or 64, LMUL = 1):
 *   vfadd.vv  vfadd.vf   funct6 000000   OPFVV / OPFVF   vd[i] = vs2[i] + (vs1[i] | f[rs1])
 *   vfsub.vv  vfsub.vf   funct6 000010   OPFVV / OPFVF   vd[i] = vs2[i] - (vs1[i] | f[rs1])
 *   vfmul.vv  vfmul.vf   funct6 100100   OPFVV / OPFVF   vd[i] = vs2[i] * (vs1[i] | f[rs1])
 *   vfmacc.vv vfmacc.vf  funct6 101100   OPFVV / OPFVF   vd[i] = (vs1[i] | f[rs1]) * vs2[i] + vd[i]  (fused)
 *   vfmv.v.f             funct6 010111   OPFVF, vs2 = 0  vd[i] = f[rs1]
 *
 * Rules (RISC-V F/D + RVV 1.0):
 *   - Rounding mode comes from frm (CSR 0x002). RNE, RTZ, RDN and RUP are supported.
 *     RMM (4) and the reserved values 5-7 are rejected and nothing is changed.
 *   - Exception flags of all active elements are ORed into fflags (sticky), using the
 *     RISC-V bit order: NV=16, DZ=8, OF=4, UF=2, NX=1.
 *   - Any NaN result of an arithmetic op is the canonical NaN (0x7FC00000 / 0x7FF8000000000000).
 *     vfmv.v.f copies the scalar bits unchanged, like the scalar fmv instructions.
 *   - For SEW = 32 the scalar is the low 32 bits of f[rs1] (Kites stores singles
 *     zero-extended in the 64-bit FP registers rather than NaN-boxed).
 */
#ifndef VECTOR_FPU_H
#define VECTOR_FPU_H

#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace Kites::vector_fpu
{

enum class Op
{
    None,
    AddVV,
    AddVF,
    SubVV,
    SubVF,
    MulVV,
    MulVF,
    MaccVV,
    MaccVF,
    MvVF
};

constexpr uint8_t OPFVV = 0b001;
constexpr uint8_t OPFVF = 0b101;

// RISC-V fflags bit order.
constexpr uint8_t FLAG_NX = 1U << 0;
constexpr uint8_t FLAG_UF = 1U << 1;
constexpr uint8_t FLAG_OF = 1U << 2;
constexpr uint8_t FLAG_DZ = 1U << 3;
constexpr uint8_t FLAG_NV = 1U << 4;

constexpr uint32_t CANONICAL_NAN32 = 0x7FC00000U;
constexpr uint64_t CANONICAL_NAN64 = 0x7FF8000000000000ULL;

/// Decodes an OP-V (opcode 1010111) floating-point instruction. Returns Op::None if unsupported.
inline Op Decode(uint32_t instruction)
{
    const uint8_t funct3 = (instruction >> 12U) & 0x7U;
    const uint8_t vs2 = (instruction >> 20U) & 0x1FU;
    const uint8_t vm = (instruction >> 25U) & 1U;
    const uint8_t funct6 = (instruction >> 26U) & 0x3FU;
    if ((instruction & 0x7FU) != 0b1010111 || vm != 1 || (funct3 != OPFVV && funct3 != OPFVF))
        return Op::None;
    const bool vv = funct3 == OPFVV;
    switch (funct6)
    {
    case 0b000000:
        return vv ? Op::AddVV : Op::AddVF;
    case 0b000010:
        return vv ? Op::SubVV : Op::SubVF;
    case 0b100100:
        return vv ? Op::MulVV : Op::MulVF;
    case 0b101100:
        return vv ? Op::MaccVV : Op::MaccVF;
    case 0b010111:
        return (!vv && vs2 == 0) ? Op::MvVF : Op::None;
    default:
        return Op::None;
    }
}

/// True if the funct3 field selects the OPFVV/OPFVF groups (used to route to this unit).
inline bool IsFloatingPoint(uint32_t instruction)
{
    const uint8_t funct3 = (instruction >> 12U) & 0x7U;
    return (instruction & 0x7FU) == 0b1010111 && (funct3 == OPFVV || funct3 == OPFVF);
}

/// True if bits 19:15 name a vector register (vs1); false if they name a scalar f register.
inline bool UsesVectorSource1(Op op)
{
    return op == Op::AddVV || op == Op::SubVV || op == Op::MulVV || op == Op::MaccVV;
}

inline bool ReadsDestination(Op op) { return op == Op::MaccVV || op == Op::MaccVF; }

/// Error text for a frm value this unit cannot honour, or nullptr when it is supported.
inline const char *RoundingModeError(uint64_t frm)
{
    if (frm <= 3)
        return nullptr;
    if (frm == 4)
        return "Vector FP: rounding mode RMM (frm=4) is not supported";
    return "Vector FP: frm holds a reserved rounding mode (5-7)";
}

/**
 * Sets the host rounding mode from frm and clears the host exception flags for the
 * lifetime of the object; restores the previous host rounding mode afterwards.
 */
class Scope
{
public:
    explicit Scope(uint64_t frm) : saved_(std::fegetround())
    {
        switch (frm)
        {
        case 0: std::fesetround(FE_TONEAREST); break;
        case 1: std::fesetround(FE_TOWARDZERO); break;
        case 2: std::fesetround(FE_DOWNWARD); break;
        case 3: std::fesetround(FE_UPWARD); break;
        default: break;
        }
        std::feclearexcept(FE_ALL_EXCEPT);
    }
    ~Scope() { std::fesetround(saved_); }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

    /// Host exceptions raised since construction, in RISC-V fflags order.
    static uint8_t Flags()
    {
        uint8_t flags = 0;
        if (std::fetestexcept(FE_INEXACT)) flags |= FLAG_NX;
        if (std::fetestexcept(FE_UNDERFLOW)) flags |= FLAG_UF;
        if (std::fetestexcept(FE_OVERFLOW)) flags |= FLAG_OF;
        if (std::fetestexcept(FE_DIVBYZERO)) flags |= FLAG_DZ;
        if (std::fetestexcept(FE_INVALID)) flags |= FLAG_NV;
        return flags;
    }

private:
    int saved_;
};

namespace detail
{
template <typename F, typename U>
U Lane(Op op, U a_bits, U b_bits, U d_bits, U canonical_nan)
{
    if (op == Op::MvVF)
        return b_bits;
    // volatile keeps the compiler from folding or moving the arithmetic across the
    // rounding-mode change made by Scope.
    F a, b, d;
    std::memcpy(&a, &a_bits, sizeof(F));
    std::memcpy(&b, &b_bits, sizeof(F));
    std::memcpy(&d, &d_bits, sizeof(F));
    volatile F va = a, vb = b, vd = d;
    F r;
    switch (op)
    {
    case Op::AddVV:
    case Op::AddVF:
        r = va + vb;
        break;
    case Op::SubVV:
    case Op::SubVF:
        r = va - vb;
        break;
    case Op::MulVV:
    case Op::MulVF:
        r = va * vb;
        break;
    case Op::MaccVV:
    case Op::MaccVF:
        r = std::fma(static_cast<F>(vb), static_cast<F>(va), static_cast<F>(vd));
        break;
    default:
        return d_bits;
    }
    volatile F vr = r;
    if (std::isnan(static_cast<F>(vr)))
        return canonical_nan;
    U out;
    F rr = vr;
    std::memcpy(&out, &rr, sizeof(F));
    return out;
}
} // namespace detail

/**
 * One lane: a = vs2[i], b = vs1[i] or the scalar, d = old vd[i], each holding SEW bits.
 * Must be called inside a Scope. Returns the SEW-bit result.
 */
inline uint64_t Compute(Op op, unsigned sew, uint64_t a, uint64_t b, uint64_t d)
{
    if (sew == 32)
        return detail::Lane<float, uint32_t>(op, static_cast<uint32_t>(a), static_cast<uint32_t>(b),
                                             static_cast<uint32_t>(d), CANONICAL_NAN32);
    return detail::Lane<double, uint64_t>(op, a, b, d, CANONICAL_NAN64);
}

struct Outcome
{
    const char *error = nullptr; ///< Non-null: nothing was changed, report this.
    uint8_t flags = 0;           ///< fflags bits raised by the active elements.
};

/**
 * Runs one instruction over elements 0..vl-1. Elements vl..end of vd are left
 * unchanged (tail undisturbed). On error, `destination` is not modified.
 *
 * @param source1 vs1 for the .vv forms (ignored for .vf / vfmv.v.f)
 * @param scalar  f[rs1] for the .vf forms and vfmv.v.f
 */
template <typename VectorRegister>
Outcome Execute(Op op, unsigned sew, size_t vl, uint64_t frm, const VectorRegister &source2,
                const VectorRegister &source1, uint64_t scalar, VectorRegister &destination)
{
    Outcome outcome;
    if (sew != 32 && sew != 64)
    {
        outcome.error = "Vector FP instructions need SEW = 32 or 64 (e32/e64)";
        return outcome;
    }
    if (op != Op::MvVF)
    {
        // vfmv.v.f does no arithmetic, so frm does not matter for it.
        if (const char *error = RoundingModeError(frm))
        {
            outcome.error = error;
            return outcome;
        }
    }
    const size_t lane_bytes = sew / 8U;
    if (vl > destination.size() / lane_bytes)
    {
        outcome.error = "Vector FP: VL exceeds VLMAX";
        return outcome;
    }
    const uint64_t lane_mask = (sew == 64) ? ~uint64_t{0} : 0xFFFFFFFFULL;
    const bool vector_source1 = UsesVectorSource1(op);
    const VectorRegister old_destination = destination;

    Scope scope(frm);
    for (size_t lane = 0; lane < vl; ++lane)
    {
        const size_t offset = lane * lane_bytes;
        uint64_t a = 0;
        uint64_t b = vector_source1 ? 0 : (scalar & lane_mask);
        uint64_t d = 0;
        for (size_t byte = 0; byte < lane_bytes; ++byte)
        {
            a |= uint64_t{source2[offset + byte]} << (byte * 8U);
            d |= uint64_t{old_destination[offset + byte]} << (byte * 8U);
            if (vector_source1)
                b |= uint64_t{source1[offset + byte]} << (byte * 8U);
        }
        const uint64_t result = Compute(op, sew, a, b, d) & lane_mask;
        for (size_t byte = 0; byte < lane_bytes; ++byte)
            destination[offset + byte] = static_cast<uint8_t>(result >> (byte * 8U));
    }
    outcome.flags = (op == Op::MvVF) ? 0 : Scope::Flags();
    return outcome;
}

} // namespace Kites::vector_fpu

#endif // VECTOR_FPU_H
