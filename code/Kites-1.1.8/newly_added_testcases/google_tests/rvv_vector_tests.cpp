// Google Tests for the vector extension.
// Each test runs one program from ../manual_tests/ on three processors
// (single cycle, 5-stage hazard + forwarding, 5-stage hazard without forwarding)
// and checks the same values that are listed in ../manual_tests/TEST_RESULTS.md.
// Run from this folder (the working directory is set by CMake / ctest).

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

using namespace Kites;

namespace
{

const std::string kDir = "../manual_tests/";

// Final state of one run.
struct Result
{
    std::string processor;
    std::vector<uint64_t> x, f;
    std::vector<RegisterFile::VectorRegister> v;
    uint64_t vl = 0, sew = 0, instructions = 0, cycles = 0;
    std::vector<uint32_t> output; // 32 words before a2 (FIR tests)

    // Element i of vector register r, read with the final SEW.
    uint64_t lane(int r, unsigned i) const
    {
        const unsigned bytes = static_cast<unsigned>(sew / 8);
        uint64_t value = 0;
        for (unsigned b = 0; b < bytes; ++b)
            value |= uint64_t{v[r][i * bytes + b]} << (8 * b);
        return value;
    }
};

template <typename VM>
Result runOn(const std::string &file, const char *name)
{
    setupVmStateDirectory();
    auto vm = std::make_unique<VM>();
    vm->LoadProgram(assemble(kDir + file));
    vm->DebugRun();
    Result r;
    r.processor = name;
    for (int i = 0; i < 32; ++i)
    {
        r.x.push_back(vm->registers_.ReadGpr(i));
        r.f.push_back(vm->registers_.ReadFpr(i));
        r.v.push_back(vm->registers_.ReadVpr(i));
    }
    r.vl = vm->registers_.ReadVl();
    r.sew = vm->registers_.ReadSew();
    r.instructions = vm->instructions_retired_;
    r.cycles = vm->cycle_s_;
    // FIR programs leave a2 just past their 32-word output; other tests do not use a2.
    if (vm->registers_.ReadGpr(12) >= 128)
    {
        const uint64_t out = vm->registers_.ReadGpr(12) - 128;
        for (int i = 0; i < 32; ++i)
            r.output.push_back(vm->memory_controller_.readWord(out + 4 * i));
    }
    return r;
}

std::vector<Result> runAll(const std::string &file)
{
    return {runOn<RVSSProcessor>(file, "single cycle"),
            runOn<RV5StageProcessorHF>(file, "5-stage hazard + forwarding"),
            runOn<RV5StageProcessorHNF>(file, "5-stage hazard, no forwarding")};
}

// Every active element of vector register r equals `value`.
void expectAll(const Result &r, int reg, uint64_t value, unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
        EXPECT_EQ(r.lane(reg, i), value) << "v" << reg << "[" << i << "] on " << r.processor;
}

// ABI register numbers used in the programs
constexpr int t0 = 5, t1 = 6, t2 = 7, t3 = 28, t4 = 29, t5 = 30, t6 = 31;

} // namespace

TEST(RvvManual, Test01_Vsetvli)
{
    for (const auto &r : runAll("test01_vsetvli.s"))
    {
        SCOPED_TRACE(r.processor);
        EXPECT_EQ(r.x[t1], 4U);  // e32, AVL 4  -> 4
        EXPECT_EQ(r.x[t2], 4U);  // e32, AVL 7  -> ceil(7/2) = 4
        EXPECT_EQ(r.x[t3], 2U);  // e64, AVL 3  -> ceil(3/2) = 2
        EXPECT_EQ(r.x[t4], 10U); // e8,  AVL 20 -> ceil(20/2) = 10
        EXPECT_EQ(r.x[t5], 16U); // e8,  AVL 40 -> VLMAX 16
        EXPECT_EQ(r.x[t6], 8U);  // e16, rs1 = x0 -> VLMAX 8
        EXPECT_EQ(r.vl, 8U);
        EXPECT_EQ(r.sew, 16U);
    }
}

TEST(RvvManual, Test02_LoadStore)
{
    for (const auto &r : runAll("test02_load_store.s"))
    {
        SCOPED_TRACE(r.processor);
        EXPECT_EQ(r.lane(1, 0), 10U);
        EXPECT_EQ(r.lane(1, 1), 20U);
        EXPECT_EQ(r.lane(1, 2), 30U);
        EXPECT_EQ(r.lane(1, 3), 40U);
        EXPECT_EQ(r.x[t3], 10U); // first word read back from the stored copy
        EXPECT_EQ(r.x[t4], 40U); // last word
    }
}

TEST(RvvManual, Test03_ElementWidths)
{
    for (const auto &r : runAll("test03_element_widths.s"))
    {
        SCOPED_TRACE(r.processor);
        ASSERT_EQ(r.sew, 64U);
        for (int reg : {1, 3, 4})
        {
            EXPECT_EQ(r.lane(reg, 0), 0xfefdfcfbfa030201ULL) << "v" << reg;
            EXPECT_EQ(r.lane(reg, 1), 0x3c32281e140a00ffULL) << "v" << reg;
        }
        EXPECT_EQ(r.lane(2, 0), 0x0e0d0c0b0a131211ULL); // each byte + 16, wraps past 255
        EXPECT_EQ(r.lane(2, 1), 0x4c42382e241a100fULL);
    }
}

TEST(RvvManual, Test04_AddSub)
{
    for (const auto &r : runAll("test04_add_sub.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 3, 8, 4);
        expectAll(r, 4, 0xfffffffe, 4); // -2
        expectAll(r, 5, 105, 4);
        expectAll(r, 6, 0xffffffa1, 4); // -95
    }
}

TEST(RvvManual, Test05_Mul)
{
    for (const auto &r : runAll("test05_mul.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 1, 300, 8);
        expectAll(r, 2, 0x5f90, 8); // 90000 mod 65536
        expectAll(r, 3, 0xfda8, 8); // -600
    }
}

TEST(RvvManual, Test06_Macc)
{
    for (const auto &r : runAll("test06_macc.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 3, 85, 4); // 2*7 + 1 = 15, then 10*7 + 15 = 85
    }
}

TEST(RvvManual, Test07_Moves)
{
    for (const auto &r : runAll("test07_moves.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 1, 42, 4);
        expectAll(r, 2, 42, 4);
        expectAll(r, 3, 0xffffffff, 4);
        expectAll(r, 4, 15, 4);
        expectAll(r, 5, 0xfffffff0, 4);
    }
}

TEST(RvvManual, Test08_TailUndisturbed)
{
    for (const auto &r : runAll("test08_tail_undisturbed.s"))
    {
        SCOPED_TRACE(r.processor);
        EXPECT_EQ(r.x[t1], 2U);
        EXPECT_EQ(r.lane(1, 0), 2U);
        EXPECT_EQ(r.lane(1, 1), 2U);
        EXPECT_EQ(r.lane(1, 2), 9U); // tail kept
        EXPECT_EQ(r.lane(1, 3), 9U);
    }
}

TEST(RvvManual, Test09_PipelineHazard)
{
    for (const auto &r : runAll("test09_pipeline_hazard.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 2, 8, 4);
        expectAll(r, 3, 96, 4);
    }
}

TEST(RvvManual, Test10_FpAddSubMul)
{
    for (const auto &r : runAll("test10_fp_add_sub_mul.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 3, 0x40600000, 4); // 3.5
        expectAll(r, 4, 0xbf000000, 4); // -0.5
        expectAll(r, 5, 0x40400000, 4); // 3.0
        expectAll(r, 6, 0x40600000, 4); // 3.5
        expectAll(r, 7, 0xbf000000, 4); // -0.5
        expectAll(r, 8, 0x40100000, 4); // 2.25
    }
}

TEST(RvvManual, Test11_FpMacc)
{
    for (const auto &r : runAll("test11_fp_macc.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 1, 0x40400000, 4); // 3.0
        expectAll(r, 2, 0x41980000, 4); // 19.0
    }
}

TEST(RvvManual, Test12_FpSpecialValues)
{
    for (const auto &r : runAll("test12_fp_special_values.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 2, 0x7fc00000, 4); // inf - inf = NaN
        expectAll(r, 3, 0x7fc00000, 4); // inf * 0 = NaN
        expectAll(r, 5, 0x7f800000, 4); // overflow -> inf
    }
}

TEST(RvvManual, Test13_FpDouble)
{
    for (const auto &r : runAll("test13_fp_double.s"))
    {
        SCOPED_TRACE(r.processor);
        expectAll(r, 2, 0x4010000000000000ULL, 2); // 4.0
        expectAll(r, 3, 0x4024000000000000ULL, 2); // 10.0
    }
}

TEST(RvvManual, Test14_IntegerFirSpeedup)
{
    const auto scalar = runAll("test14a_fir_scalar_int.s");
    const auto vector = runAll("test14b_fir_vector_int.s");
    for (size_t p = 0; p < scalar.size(); ++p)
    {
        SCOPED_TRACE(scalar[p].processor);
        for (int i = 0; i < 32; ++i)
        {
            EXPECT_EQ(scalar[p].output[i], static_cast<uint32_t>(17 * i + 51)) << "scalar out[" << i << "]";
            EXPECT_EQ(vector[p].output[i], static_cast<uint32_t>(17 * i + 51)) << "vector out[" << i << "]";
        }
        EXPECT_EQ(scalar[p].instructions, 1319U);
        EXPECT_EQ(vector[p].instructions, 319U);
    }
    EXPECT_EQ(scalar[1].cycles, 1704U); // hazard + forwarding
    EXPECT_EQ(vector[1].cycles, 416U);
    EXPECT_EQ(scalar[2].cycles, 2446U); // hazard, no forwarding
    EXPECT_EQ(vector[2].cycles, 560U);
}

TEST(RvvManual, Test15_FloatFirSpeedup)
{
    const auto scalar = runAll("test15a_fir_scalar_fp.s");
    const auto vector = runAll("test15b_fir_vector_fp.s");
    for (size_t p = 0; p < scalar.size(); ++p)
    {
        SCOPED_TRACE(scalar[p].processor);
        for (int i = 0; i < 32; ++i)
        {
            const float expected = 4.25f * static_cast<float>(i) + 13.0f;
            float a, b;
            std::memcpy(&a, &scalar[p].output[i], 4);
            std::memcpy(&b, &vector[p].output[i], 4);
            EXPECT_EQ(a, expected) << "scalar out[" << i << "]";
            EXPECT_EQ(b, expected) << "vector out[" << i << "]";
        }
        EXPECT_EQ(scalar[p].instructions, 1191U);
        EXPECT_EQ(vector[p].instructions, 320U);
    }
    EXPECT_EQ(scalar[1].cycles, 1576U);
    EXPECT_EQ(vector[1].cycles, 417U);
    EXPECT_EQ(scalar[2].cycles, 2446U);
    EXPECT_EQ(vector[2].cycles, 561U);
}

// Undo after every step brings the vector registers, VL and SEW back.
template <typename VM>
void undoEveryStep(const char *name, bool singleCycle)
{
    SCOPED_TRACE(name);
    setupVmStateDirectory();
    auto vm = std::make_unique<VM>();
    vm->LoadProgram(assemble(kDir + "test06_macc.s"));
    auto snapshot = [&] {
        std::vector<RegisterFile::VectorRegister> s;
        for (int i = 0; i < 8; ++i)
            s.push_back(vm->registers_.ReadVpr(i));
        return std::make_pair(s, vm->registers_.ReadVl());
    };
    for (int step = 0; step < 15; ++step)
    {
        // The single-cycle processor stops at the end of the program (nothing left to undo).
        if (singleCycle && vm->program_counter_ >= vm->program_size_)
            break;
        const auto before = snapshot();
        vm->Step();
        const auto after = snapshot();
        vm->Undo();
        EXPECT_TRUE(snapshot() == before) << "undo after step " << step;
        vm->Step();
        EXPECT_TRUE(snapshot() == after) << "redo of step " << step;
    }
}

TEST(RvvManual, TestU_UndoRestoresVectorState)
{
    undoEveryStep<RVSSProcessor>("single cycle", true);
    undoEveryStep<RV5StageProcessorHF>("5-stage hazard + forwarding", false);
}

// ===========================================================================
// 1008 generated programs (tests 17 ... 1024)
//
// Each test builds one random vector program from a fixed seed, works out the
// correct answer with a small reference model written here (not the simulator's
// code), runs the program on all five processors, and checks:
//   - v1 ... v7 after the program,
//   - the values the program stored to memory with vse,
//   - VL, and fflags for floating-point programs.
// Even seeds use the integer instructions (SEW 8/16/32/64), odd seeds use the
// floating-point instructions (SEW 32/64) with a random rounding mode.
// The no-hazard pipelines get 3 nops after every instruction, as they need.
// ===========================================================================
#include <cfenv>
#include <cmath>
#include <sstream>

#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"

namespace
{

constexpr uint64_t kData = 0x10000000; // v1..v4 are loaded from here
constexpr uint64_t kOut = 0x10000100;  // v1..v7 are stored here at the end

struct Rng
{
    uint64_t s;
    uint64_t next()
    {
        uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    uint64_t below(uint64_t n) { return next() % n; }
};

using VReg = RegisterFile::VectorRegister;

struct Program
{
    bool fp = false;
    unsigned sew = 32;
    uint64_t vl = 0, frm = 0;
    std::vector<std::string> lines;
    std::vector<uint8_t> data;              // 64 bytes at kData
    std::array<uint64_t, 32> x{}, f{};      // preset scalar registers
    std::array<VReg, 8> expected{};         // v0..v7 at the end
    uint8_t expectedFlags = 0;
};

uint64_t laneOf(const VReg &r, unsigned eb, uint64_t i)
{
    uint64_t v = 0;
    for (unsigned b = 0; b < eb; ++b)
        v |= uint64_t{r[i * eb + b]} << (8 * b);
    return v;
}
void setLane(VReg &r, unsigned eb, uint64_t i, uint64_t v)
{
    for (unsigned b = 0; b < eb; ++b)
        r[i * eb + b] = static_cast<uint8_t>(v >> (8 * b));
}

uint64_t intValue(Rng &r, unsigned sew)
{
    static const uint64_t special[] = {0, 1, 2, 0x7f, 0x80, 0xff, 0x7fff, 0x8000, 0xffff,
                                       0x7fffffff, 0x80000000, 0xffffffff, ~0ULL, 0x8000000000000000ULL};
    const uint64_t mask = sew == 64 ? ~0ULL : (1ULL << sew) - 1;
    return (r.below(4) == 0 ? special[r.below(14)] : r.next()) & mask;
}

uint64_t fpValue(Rng &r, unsigned sew)
{
    static const uint32_t s32[] = {0, 0x80000000, 0x3f800000, 0xbf800000, 0x40000000, 0x3f000000,
                                   0x7f800000, 0xff800000, 0x7fc00000, 0x7f800001, 0x7f7fffff,
                                   0x00800000, 0x00000001, 0x3dcccccd};
    static const uint64_t s64[] = {0, 0x8000000000000000ULL, 0x3ff0000000000000ULL, 0x4000000000000000ULL,
                                   0x7ff0000000000000ULL, 0xfff0000000000000ULL, 0x7ff8000000000000ULL,
                                   0x7ff0000000000001ULL, 0x7fefffffffffffffULL, 0x0010000000000000ULL,
                                   1, 0x3fb999999999999aULL};
    if (r.below(3) == 0)
        return sew == 32 ? s32[r.below(14)] : s64[r.below(12)];
    if (sew == 32) // random sign, exponent near 1.0, random mantissa
        return (r.below(2) << 31) | ((112 + r.below(32)) << 23) | (r.next() & 0x7fffff);
    return (r.below(2) << 63) | ((1008 + r.below(32)) << 52) | (r.next() & 0xfffffffffffffULL);
}

// Reference for one floating-point element (same rules as the RISC-V spec:
// canonical NaN result, fused multiply-add).
template <typename F, typename U>
U fpLane(const std::string &op, U a, U b, U d, U canonical)
{
    if (op == "vfmv.v.f")
        return b;
    F fa, fb, fd;
    std::memcpy(&fa, &a, sizeof(F));
    std::memcpy(&fb, &b, sizeof(F));
    std::memcpy(&fd, &d, sizeof(F));
    volatile F x = fa, y = fb, z = fd;
    F r;
    if (op.rfind("vfadd", 0) == 0)
        r = x + y;
    else if (op.rfind("vfsub", 0) == 0)
        r = x - y;
    else if (op.rfind("vfmul", 0) == 0)
        r = x * y;
    else
        r = std::fma(static_cast<F>(y), static_cast<F>(x), static_cast<F>(z));
    volatile F rr = r;
    if (std::isnan(static_cast<F>(rr)))
        return canonical;
    U out;
    F t = rr;
    std::memcpy(&out, &t, sizeof(F));
    return out;
}

uint8_t hostFlags()
{
    uint8_t f = 0;
    if (std::fetestexcept(FE_INEXACT)) f |= 1;
    if (std::fetestexcept(FE_UNDERFLOW)) f |= 2;
    if (std::fetestexcept(FE_OVERFLOW)) f |= 4;
    if (std::fetestexcept(FE_DIVBYZERO)) f |= 8;
    if (std::fetestexcept(FE_INVALID)) f |= 16;
    return f;
}

Program makeProgram(int seed)
{
    Rng r{static_cast<uint64_t>(seed) * 0x2545F4914F6CDD1DULL + 77};
    Program p;
    p.fp = (seed % 2) == 1;
    static const unsigned intSew[] = {8, 16, 32, 64};
    p.sew = p.fp ? (r.below(2) ? 32 : 64) : intSew[r.below(4)];
    const unsigned eb = p.sew / 8;
    const uint64_t vlmax = 16 / eb, avl = r.below(2 * vlmax + 2);
    p.vl = avl <= vlmax ? avl : avl < 2 * vlmax ? (avl + 1) / 2 : vlmax;
    p.frm = p.fp ? r.below(4) : 0;
    const uint64_t mask = p.sew == 64 ? ~0ULL : (1ULL << p.sew) - 1;
    const std::string e = std::to_string(p.sew);

    // memory for v1..v4 and preset scalars
    p.data.resize(64);
    for (unsigned k = 0; k < 4; ++k)
        for (uint64_t i = 0; i < vlmax; ++i)
        {
            const uint64_t val = p.fp ? fpValue(r, p.sew) : intValue(r, p.sew);
            for (unsigned b = 0; b < eb; ++b)
                p.data[k * 16 + i * eb + b] = static_cast<uint8_t>(val >> (8 * b));
        }
    for (int i = 5; i <= 9; ++i)
        p.x[i] = p.fp ? fpValue(r, p.sew) : r.next();
    for (int i = 1; i <= 5; ++i)
        p.f[i] = fpValue(r, p.sew);

    std::array<VReg, 8> v{};
    p.lines = {"lui x31, 0x10000", "addi x10, x0, " + std::to_string(avl),
               "vsetvli x11, x10, e" + e + ", m1, tu, mu"};
    for (int k = 1; k <= 4; ++k)
    {
        p.lines.push_back("addi x12, x31, " + std::to_string((k - 1) * 16));
        p.lines.push_back("vle" + e + ".v v" + std::to_string(k) + ", (x12)");
        for (uint64_t i = 0; i < p.vl; ++i)
            setLane(v[k], eb, i, laneOf(*reinterpret_cast<const VReg *>(&p.data[(k - 1) * 16]), eb, i));
    }

    static const char *intOps[] = {"vadd.vv", "vadd.vx", "vsub.vv", "vsub.vx", "vmul.vv", "vmul.vx",
                                   "vmacc.vv", "vmacc.vx", "vmv.v.x", "vmv.v.v", "vmv.v.i"};
    static const char *fpOps[] = {"vfadd.vv", "vfadd.vf", "vfsub.vv", "vfsub.vf", "vfmul.vv",
                                  "vfmul.vf", "vfmacc.vv", "vfmacc.vf", "vfmv.v.f"};
    std::array<uint64_t, 32> x = p.x, f = p.f;
    const int saved = std::fegetround();
    std::fesetround(p.frm == 0 ? FE_TONEAREST : p.frm == 1 ? FE_TOWARDZERO : p.frm == 2 ? FE_DOWNWARD : FE_UPWARD);
    uint8_t flags = 0;

    for (int n = 0; n < 10; ++n)
    {
        const std::string op = p.fp ? fpOps[r.below(9)] : intOps[r.below(11)];
        const int vd = 1 + static_cast<int>(r.below(7)), va = 1 + static_cast<int>(r.below(7)),
                  vb = 1 + static_cast<int>(r.below(7)), xs = 5 + static_cast<int>(r.below(5)),
                  fs = 1 + static_cast<int>(r.below(5));
        const int64_t imm = static_cast<int64_t>(r.below(32)) - 16;

        // Sometimes the scalar operand is changed by the instruction just before
        // (tests the pipeline bypass of a value that has not been written back yet).
        if (r.below(3) == 0)
        {
            if (!p.fp)
            {
                const int64_t add = static_cast<int64_t>(r.below(200)) - 100;
                p.lines.push_back("addi x" + std::to_string(xs) + ", x" + std::to_string(xs) + ", " +
                                  std::to_string(add));
                x[xs] += static_cast<uint64_t>(add);
            }
            else
            {
                p.lines.push_back(std::string(p.sew == 32 ? "fmv.w.x f" : "fmv.d.x f") + std::to_string(fs) +
                                  ", x" + std::to_string(xs));
                f[fs] = p.sew == 32 ? (x[xs] & 0xffffffffULL) : x[xs];
            }
        }

        const std::string V = "v" + std::to_string(vd), A = "v" + std::to_string(va), B = "v" + std::to_string(vb),
                          X = "x" + std::to_string(xs), Fr = "f" + std::to_string(fs);
        if (op == "vmacc.vv" || op == "vfmacc.vv")
            p.lines.push_back(op + " " + V + ", " + B + ", " + A);
        else if (op == "vmacc.vx")
            p.lines.push_back(op + " " + V + ", " + X + ", " + A);
        else if (op == "vfmacc.vf")
            p.lines.push_back(op + " " + V + ", " + Fr + ", " + A);
        else if (op == "vmv.v.x")
            p.lines.push_back(op + " " + V + ", " + X);
        else if (op == "vmv.v.v")
            p.lines.push_back(op + " " + V + ", " + B);
        else if (op == "vmv.v.i")
            p.lines.push_back(op + " " + V + ", " + std::to_string(imm));
        else if (op == "vfmv.v.f")
            p.lines.push_back(op + " " + V + ", " + Fr);
        else if (op.back() == 'x')
            p.lines.push_back(op + " " + V + ", " + A + ", " + X);
        else if (op.back() == 'f')
            p.lines.push_back(op + " " + V + ", " + A + ", " + Fr);
        else
            p.lines.push_back(op + " " + V + ", " + A + ", " + B);

        const VReg ra = v[va], rb = v[vb], rd = v[vd];
        for (uint64_t i = 0; i < p.vl; ++i)
        {
            const uint64_t a = laneOf(ra, eb, i), d = laneOf(rd, eb, i), bv = laneOf(rb, eb, i);
            uint64_t res;
            if (!p.fp)
            {
                const uint64_t s = x[xs] & mask;
                if (op == "vadd.vv") res = a + bv;
                else if (op == "vadd.vx") res = a + s;
                else if (op == "vsub.vv") res = a - bv;
                else if (op == "vsub.vx") res = a - s;
                else if (op == "vmul.vv") res = a * bv;
                else if (op == "vmul.vx") res = a * s;
                else if (op == "vmacc.vv") res = bv * a + d;
                else if (op == "vmacc.vx") res = s * a + d;
                else if (op == "vmv.v.x") res = s;
                else if (op == "vmv.v.v") res = bv;
                else res = static_cast<uint64_t>(imm);
            }
            else
            {
                const uint64_t b = (op.back() == 'v') ? bv : (f[fs] & mask);
                std::feclearexcept(FE_ALL_EXCEPT);
                res = p.sew == 32 ? fpLane<float, uint32_t>(op, static_cast<uint32_t>(a), static_cast<uint32_t>(b),
                                                            static_cast<uint32_t>(d), 0x7fc00000U)
                                  : fpLane<double, uint64_t>(op, a, b, d, 0x7ff8000000000000ULL);
                if (op != "vfmv.v.f")
                    flags |= hostFlags();
            }
            setLane(v[vd], eb, i, res & mask);
        }
    }
    std::fesetround(saved);

    for (int k = 1; k <= 7; ++k)
    {
        p.lines.push_back("addi x12, x31, " + std::to_string(0x100 + (k - 1) * 16));
        p.lines.push_back("vse" + e + ".v v" + std::to_string(k) + ", (x12)");
    }
    p.expected = v;
    p.expectedFlags = flags;
    return p;
}

template <typename VM>
void checkProgram(const Program &p, const char *name, bool padded)
{
    SCOPED_TRACE(name);
    setupVmStateDirectory();
    auto vm = std::make_unique<VM>();
    std::ostringstream src, listing;
    for (const auto &l : p.lines)
    {
        src << l << "\n" << (padded ? "nop\nnop\nnop\n" : "");
        listing << "  " << l << "\n";
    }
    std::istringstream in(src.str());
    vm->LoadProgram(assemble(in));
    for (int i = 5; i <= 9; ++i)
        vm->registers_.WriteGpr(i, p.x[i]);
    for (int i = 1; i <= 5; ++i)
        vm->registers_.WriteFpr(i, p.f[i]);
    vm->registers_.WriteCsr(0x002, p.frm);
    for (size_t i = 0; i < p.data.size(); ++i)
        vm->memory_controller_.writeByte(kData + i, p.data[i]);
    for (uint64_t a = 0; a < 7 * 16; ++a) // known contents where the results are stored
        vm->memory_controller_.writeByte(kOut + a, 0);
    vm->DebugRun();

    const unsigned eb = p.sew / 8;
    EXPECT_EQ(vm->registers_.ReadVl(), p.vl) << listing.str();
    for (int k = 1; k <= 7; ++k)
    {
        EXPECT_TRUE(vm->registers_.ReadVpr(k) == p.expected[k]) << "v" << k << " differs\n" << listing.str();
        for (uint64_t i = 0; i < p.vl; ++i) // stored copy of the active elements
        {
            uint64_t stored = 0;
            for (unsigned b = 0; b < eb; ++b)
                stored |= uint64_t{vm->memory_controller_.readByte(kOut + (k - 1) * 16 + i * eb + b)} << (8 * b);
            EXPECT_EQ(stored, laneOf(p.expected[k], eb, i)) << "stored v" << k << "[" << i << "]";
        }
    }
    if (p.fp)
        EXPECT_EQ(vm->registers_.ReadCsr(0x001), uint64_t{p.expectedFlags}) << "fflags\n" << listing.str();
}

class RvvRandom : public ::testing::TestWithParam<int>
{
};

} // namespace

TEST_P(RvvRandom, ProgramMatchesReferenceOnAllProcessors)
{
    const Program p = makeProgram(GetParam());
    checkProgram<RVSSProcessor>(p, "single cycle", false);
    checkProgram<RV5StageProcessorHF>(p, "5-stage hazard + forwarding", false);
    checkProgram<RV5StageProcessorHNF>(p, "5-stage hazard, no forwarding", false);
    checkProgram<RV5StageProcessorNHF>(p, "5-stage no hazard, forwarding (nops added)", true);
    checkProgram<RV5StageProcessorNHNF>(p, "5-stage no hazard, no forwarding (nops added)", true);
}

// 16 hand-written tests above + 1008 generated tests = 1024 tests in this file.
INSTANTIATE_TEST_SUITE_P(Generated, RvvRandom, ::testing::Range(0, 1008));
