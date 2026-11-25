#pragma once
#include "processor/pipeline_registers.h"
#include "processor/processor_base.h"
#include "processor/processor_manager.h"
#include "rv5s_control_unit.h"

namespace Kites
{
struct PipelineRegisterChange
{
    IF_ID_Register old_if_id_reg;
    IF_ID_Register new_if_id_reg;

    ID_EX_Register old_id_ex_reg;
    ID_EX_Register new_id_ex_reg;

    EX_MEM_Register old_ex_mem_reg;
    EX_MEM_Register new_ex_mem_reg;

    MEM_WB_Register old_mem_wb_reg;
    MEM_WB_Register new_mem_wb_reg;
};

struct RV5StageStepDelta : public StepDelta
{
    PipelineRegisterChange pipeline_register_change;
    // vector_register_changes and old/new vl/vtype are inherited from StepDelta.
};

class RV5StageVM_Base : public ProcessorBase
{
  public:
    RV5StageVM_Base() = default;
    ~RV5StageVM_Base() = default;

    // --- VM Control Functions ---

    void PrintType()
    {
        std::cout << "rv5s_vm" << std::endl;
    }
    void DebugRun() override;
    void Reset() override;
    [[nodiscard]] unsigned int PipelineFillCycles() const override { return 4; }
    // A taken branch is resolved in MEM and squashes three slots.
    [[nodiscard]] unsigned int BranchPenaltyCycles() const override { return 3; }
    // A jal/jalr is resolved in EX and squashes two slots.
    [[nodiscard]] unsigned int JumpPenaltyCycles() const override { return 2; }
    void Undo() override;
    void Redo() override;

  protected:
    // Pipeline Registers
    IF_ID_Register if_id_reg_;
    ID_EX_Register id_ex_reg_;
    EX_MEM_Register ex_mem_reg_;
    MEM_WB_Register mem_wb_reg_;

    // The control unit for the pipeline
    RV5SControlUnit control_unit_;

    std::stack<RV5StageStepDelta> undo_stack_{};
    std::stack<RV5StageStepDelta> redo_stack_{};
    RV5StageStepDelta current_delta_;

    // Set when EX/MEM redirected the PC this cycle; consumed by fetch_and_advance_pc().
    bool pc_redirected_ = false;

    void memory_writeback();
    void memory_writeback_float();
    void memory_writeback_double();

    void register_write_back(const uint64_t &write_data);
    void memory_read();
    // void memory_read_float();
    // void memory_read_double();

    void begin_step_delta();
    void finalize_step_delta();

    void print_pipeline_registers_debug();
    bool is_pipeline_drained() const;
    void setProcessorState() override;
    void Run() override; // run debug run adn reset are same across all rv5s vms
    bool StepAndNotify() override;

    // --- Private methods for each pipeline stage ---
    virtual void pipeline_fetch() = 0;

    void redirect_pc(uint64_t target);
    void fetch_and_advance_pc(bool stalled);

    void pipeline_decode();

    virtual void pipeline_execute() = 0;
    virtual uint64_t pipeline_execute_float() = 0;
    virtual uint64_t pipeline_execute_double() = 0;

    uint64_t execute_float();
    uint64_t execute_double();

    void execute_csr() {};

    void pipeline_memory();
    void pipeline_writeback();
    void pipeline_writeback_float();
    void pipeline_writeback_double();

    // Shared RVV execution helpers used by all four five-stage processor variants.
    bool execute_vector_instruction(uint32_t instruction, uint64_t &alu_result);
    void execute_vector_memory();
    void write_vector_register_tracked(size_t index,
                                       const RegisterFile::VectorRegister &value);
    uint64_t read_vector_scalar_gpr(size_t index) const;
    // Scalar f-register operand of a .vf instruction, bypassing an FP producer in EX/MEM.
    uint64_t read_vector_scalar_fpr(size_t index) const;
    // True for F/D instructions whose destination is an x register.
    static bool fp_writes_gpr(uint32_t instruction);
    // Runs vfadd/vfsub/vfmul/vfmacc (.vv/.vf) and vfmv.v.f in EX.
    void execute_vector_float(uint32_t instruction, uint8_t vd, uint8_t rs1, uint8_t vs2);

    // --- Specialized handler functions (called from pipeline stages) ---
    virtual void handle_syscall() = 0;
};
}//namespace Kites
