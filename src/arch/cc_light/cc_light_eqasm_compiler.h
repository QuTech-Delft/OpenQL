/** \file
 * CC-light eQASM compiler implementation.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/pair.h"
#include "utils/vec.h"
#include "utils/map.h"
#include "program.h"
#include "platform.h"
#include "ir.h"
#include "arch/cc_light/cc_light_eqasm.h"
#include "eqasm_compiler.h"

namespace ql {
namespace arch {

// eqasm code : set of cc_light_eqasm instructions
typedef utils::Vec<cc_light_eqasm_instr_t> eqasm_t;

typedef utils::Vec<utils::UInt> qubit_set_t;
typedef utils::Pair<utils::UInt, utils::UInt> qubit_pair_t;
typedef utils::Vec<qubit_pair_t> qubit_pair_set_t;

const utils::UInt MAX_S_REG = 32;
const utils::UInt MAX_T_REG = 64;

QL_GLOBAL extern utils::UInt CurrSRegCount;
QL_GLOBAL extern utils::UInt CurrTRegCount;

class Mask {
public:
    utils::UInt regNo = 0;
    utils::Str regName;
    qubit_set_t squbits;
    qubit_pair_set_t dqubits;

    Mask() = default;
    explicit Mask(const qubit_set_t &qs);
    Mask(const utils::Str &rn, const qubit_set_t &qs);
    explicit Mask(const qubit_pair_set_t &qps);
};

class MaskManager {
private:
    utils::Map<utils::UInt,Mask> SReg2Mask;
    utils::Map<qubit_set_t,Mask> QS2Mask;

    utils::Map<utils::UInt,Mask> TReg2Mask;
    utils::Map<qubit_pair_set_t,Mask> QPS2Mask;

public:
    MaskManager();
    ~MaskManager();
    utils::UInt getRegNo(qubit_set_t &qs);
    utils::UInt getRegNo(qubit_pair_set_t &qps);
    utils::Str getRegName(qubit_set_t &qs);
    utils::Str getRegName(qubit_pair_set_t &qps);
    utils::Str getMaskInstructions();
};

class classical_cc : public gate {
public:
    cmat_t m;
    // utils::Int imm_value;
    classical_cc(const utils::Str &operation, const utils::Vec<utils::UInt> &opers, utils::Int ivalue = 0);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

utils::Str classical_instruction2qisa(classical_cc *classical_in);

// FIXME HvS cc_light_instr is name of attribute in json file, in gate: arch_operation_name, here in instruction_map?
// FIXME HvS attribute of gate or just in json? Generalization to arch_operation_name is unnecessary
utils::Str get_cc_light_instruction_name(const utils::Str &id, const quantum_platform &platform);

utils::Str ir2qisa(quantum_kernel &kernel, const quantum_platform &platform, MaskManager &gMaskManager);

/**
 * cclight eqasm compiler
 */
class cc_light_eqasm_compiler : public eqasm_compiler {
public:

    cc_light_eqasm_program_t cc_light_eqasm_instructions;
    utils::UInt total_exec_time = 0;

public:

    // FIXME: should be private
    static utils::Str get_qisa_prologue(const quantum_kernel &k);
    static utils::Str get_qisa_epilogue(const quantum_kernel &k);

    void ccl_decompose_pre_schedule(quantum_program *programp, const quantum_platform &platform, const utils::Str &passname);
    void ccl_decompose_post_schedule(quantum_program *programp, const quantum_platform &platform, const utils::Str &passname);
    static void ccl_decompose_post_schedule_bundles(ir::bundles_t &bundles_dst, const quantum_platform &platform);
    static void map(quantum_program *programp, const quantum_platform &platform, const utils::Str &passname, utils::Str *mapStatistics);

    // cc_light_instr is needed by some cc_light backend passes and by cc_light resource_management:
    // - each bundle section will only have gates with the same cc_light_instr name; prepares for SIMD/SOMQ
    // - in resource management with VSMs, gates with same cc_light_instr can use same QWG in parallel
    // arch_operation_name is attempt to generalize this but is only in custom gate;
    //   so using default gates in a context where arch_operation_name is needed, would crash (e.g. wait gate)
    // it depends on that a primitive gate is one-to-one with a qisa instruction;
    //   this is something done by design now but perhaps not future-proof, e.g. towards an other backend for e.g. spin qubits
    //
    // FIXME HvS this mess must be cleaned up; so I didn't touch it further
    //
    // perhaps can be replaced by semantic definition (e.g. x90 :=: ( type=ROTATION axis=X angle=90 ) )
    // and check on equality of these instead
    // but what if there are two x90s, with different physical attributes (e.g. different amplitudes?)? Does this happen?

    static void ccl_prep_code_generation(quantum_program* programp, const quantum_platform& platform, const utils::Str &passname);
    void write_quantumsim_script(quantum_program* programp, const quantum_platform& platform, const utils::Str &passname);

    /**
     * program-level compilation of qasm to cc_light_eqasm
     */
    static void compile(const utils::Str &prog_name, circuit &ckt, const quantum_platform &platform);

    // kernel level compilation
    void compile(quantum_program *programp, const quantum_platform &platform) override;

    /**
     * decompose
     */
    // decompose meta-instructions
    static void ccl_decompose_pre_schedule_kernel(quantum_kernel &kernel, const quantum_platform &platform);

    // qisa_code_generation pass
    // generates qisa from IR
    static void qisa_code_generation(quantum_program *programp, const quantum_platform &platform, const utils::Str &passname);

private:
    // write cc_light scheduled bundles for quantumsim
    // when cc_light independent, it should be extracted and put in src/quantumsim.h
    static void write_quantumsim_program(quantum_program *programp, utils::UInt num_qubits, const quantum_platform &platform, const utils::Str &suffix);
};

} // namespace arch
} // namespace ql
