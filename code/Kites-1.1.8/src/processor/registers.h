/**
 * @file registers.h
 * @brief Contains the definition of the RegisterFile class for managing registers in the simulator.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <QObject>
#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Kites
{
/**
 * @brief Represents a register file containing integer, floating-point, and vector registers.
 */
class RegisterFile : public QObject
{
    Q_OBJECT
  public:
    static constexpr size_t NUM_GPR = 32;
    static constexpr size_t NUM_FPR = 32;
    static constexpr size_t NUM_VPR = 32;
    static constexpr size_t VLEN_BITS = 128;
    static constexpr size_t VLEN_BYTES = VLEN_BITS / 8;
    using VectorRegister = std::array<uint8_t, VLEN_BYTES>;

  private:
    static constexpr size_t NUM_CSR = 4096; ///< Number of Control and Status Registers (CSR).

    std::array<uint64_t, NUM_GPR> gpr_ = {}; ///< Array for storing GPR values.
    std::array<uint64_t, NUM_FPR> fpr_ = {}; ///< Array for storing FPR values.
    std::array<VectorRegister, NUM_VPR> vpr_ = {}; ///< 32 fixed-width vector registers.
    std::array<uint64_t, NUM_CSR> csr_ = {}; ///< Array for storing CSR values.

    size_t vl_ = 0;
    uint64_t vtype_ = 0;
    size_t sew_ = 8;
    size_t lmul_ = 1;
    bool tail_agnostic_ = false;
    bool mask_agnostic_ = false;

  public:
    /**
     * @brief Enum representing the type of a register.
     */
    enum class RegisterType
    {
        INVALID,        ///< Invalid register type.
        INTEGER,        ///< General-purpose integer register.
        FLOATING_POINT, ///< Floating-point register.
        VECTOR,         ///< Vector register.
        CSR             ///< Control and Status Register (CSR).
    };

    explicit RegisterFile(QObject *parent = nullptr);
    virtual ~RegisterFile() = default;

    void Reset();

    /**
     * @brief Reads the value of a General-Purpose Register (GPR).
     * @param reg The index of the GPR to read.
     * @return The value of the GPR at the specified index.
     */
    [[nodiscard]] uint64_t ReadGpr(size_t reg) const;

    /**
     * @brief Writes a value to a General-Purpose Register (GPR).
     * @param reg The index of the GPR to write.
     * @param value The value to write.
     */
    void WriteGpr(size_t reg, uint64_t value);

    /**
     * @brief Reads the value of a Floating-Point Register (FPR).
     * @param reg The index of the FPR to read.
     * @return The value of the FPR at the specified index.
     */
    [[nodiscard]] uint64_t ReadFpr(size_t reg) const;

    /**
     * @brief Writes a value to a Floating-Point Register (FPR).
     * @param reg The index of the FPR to write.
     * @param value The value to write.
     */
    void WriteFpr(size_t reg, uint64_t value);

    [[nodiscard]] uint64_t ReadCsr(size_t reg) const;

    void WriteCsr(size_t reg, uint64_t value);

    /**
     * @brief Retrieves the values of all General-Purpose Registers (GPR).
     * @return A vector containing the values of all GPRs.
     */
    [[nodiscard]] std::vector<uint64_t> GetGprValues() const;

    /**
     * @brief Retrieves the values of all Floating-Point Registers (FPR).
     * @return A vector containing the values of all FPRs.
     */
    [[nodiscard]] std::vector<uint64_t> GetFprValues() const;

    [[nodiscard]] VectorRegister ReadVpr(size_t reg) const;
    void WriteVpr(size_t reg, const VectorRegister &value);
    [[nodiscard]] std::vector<VectorRegister> GetVprValues() const;

    // Decode the 11-bit vtype immediate and apply the RVV AVL-to-VL rule.
    // This Phase 1 implementation supports integer LMUL values m1, m2, m4 and m8.
    [[nodiscard]] size_t ConfigureVector(uint64_t avl, uint64_t vtype);
    // Put back a vl/vtype pair previously read with ReadVl()/ReadVtype() (used by undo/redo).
    void RestoreVectorConfiguration(size_t vl, uint64_t vtype);
    [[nodiscard]] size_t ReadVl() const noexcept { return vl_; }
    [[nodiscard]] uint64_t ReadVtype() const noexcept { return vtype_; }
    [[nodiscard]] size_t ReadSew() const noexcept { return sew_; }
    [[nodiscard]] size_t ReadLmul() const noexcept { return lmul_; }
    [[nodiscard]] bool IsTailAgnostic() const noexcept { return tail_agnostic_; }
    [[nodiscard]] bool IsMaskAgnostic() const noexcept { return mask_agnostic_; }

    void ModifyRegister(const std::string &reg_name, uint64_t value);

  signals:
    void updateRegister(size_t regIndex, uint64_t value);
    void updateFRegister(size_t regIndex, uint64_t value);
    void updateVectorRegister(size_t regIndex);
    void updateVectorConfiguration(size_t vl, uint64_t vtype, size_t sew, size_t lmul);
    void registerResetSignal();
};

extern const std::unordered_set<std::string> valid_general_purpose_registers;

extern const std::unordered_set<std::string> valid_floating_point_registers;

extern const std::unordered_set<std::string> valid_vector_registers;

extern const std::unordered_set<std::string> valid_csr_registers;

extern const std::unordered_map<std::string, int> csr_to_address;

/**
 * @brief Map of register aliases to their actual names.
 */
extern const std::unordered_map<std::string, std::string> reg_alias_to_name;

bool IsValidGeneralPurposeRegister(const std::string &reg);

bool IsValidFloatingPointRegister(const std::string &reg);

bool IsValidVectorRegister(const std::string &reg);

bool IsValidCsr(const std::string &reg);
}//namespace Kites
#endif // REGISTERS_H