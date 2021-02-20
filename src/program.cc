/** \file
 * Quantum program abstraction implementation.
 */

#include "program.h"

#include "utils/filesystem.h"
#include "compiler.h"
#include "options.h"
#include "interactionMatrix.h"
#include "scheduler.h"
#include "optimizer.h"
#include "decompose_toffoli.h"
#include "clifford.h"
#include "write_sweep_points.h"
#include "arch/cc_light/cc_light_eqasm_compiler.h"
#include "arch/cc/eqasm_backend_cc.h"

static unsigned long phi_node_count = 0;    // FIXME: number across quantum_program instances

namespace ql {

using namespace utils;

/**
 * @brief   Quantum program constructor
 * @param   n   Name of the program
 */
quantum_program::quantum_program(const Str &n) : name(n) {
    platformInitialized = false;
    QL_DOUT("Constructor for quantum_program:  " << n);
}

quantum_program::quantum_program(
    const Str &n,
    const quantum_platform &platf,
    UInt nqubits,
    UInt ncregs,
    UInt nbregs
) :
    name(n),
    platform(platf),
    qubit_count(nqubits),
    creg_count(ncregs),
    breg_count(nbregs)
{
    default_config = true;
    needs_backend_compiler = true;
    platformInitialized = true;
    eqasm_compiler_name = platform.eqasm_compiler_name;
    backend_compiler    = NULL;
    if (eqasm_compiler_name.empty()) {
        QL_FATAL("eqasm compiler name must be specified in the hardware configuration file !");
    } else if (eqasm_compiler_name == "none") {
        needs_backend_compiler = false;
    } else if (eqasm_compiler_name == "qx") {
        // at the moment no qx specific thing is done
        needs_backend_compiler = false;;
    } else if (eqasm_compiler_name == "cc_light_compiler") {
        backend_compiler = new arch::cc_light_eqasm_compiler();
    } else if (eqasm_compiler_name == "eqasm_backend_cc") {
        backend_compiler = new eqasm_backend_cc();
    } else {
        QL_FATAL("the '" << eqasm_compiler_name << "' eqasm compiler backend is not suported !");
    }

    if (qubit_count > platform.qubit_number) {
        QL_FATAL("number of qubits requested in program '" + to_string(qubit_count) + "' is greater than the qubits available in platform '" + to_string(platform.qubit_number) + "'" );
    }

    // report/write_qasm initialization
    report_init(this, platform);
}

void quantum_program::add(const quantum_kernel &k) {
    // check sanity of supplied qubit/classical operands for each gate
    const circuit &kc = k.get_circuit();
    for (auto &g : kc) {
        auto &gate_operands = g->operands;
        auto &gname = g->name;
        auto gtype = g->type();
        for (auto &op : gate_operands) {
            if (
                ((gtype == __classical_gate__) && (op >= creg_count)) ||
                ((gtype != __classical_gate__) && (op >= qubit_count))
            ) {
                 QL_FATAL("Out of range operand(s) for operation: '" << gname <<
                                                                     "' (op=" << op <<
                                                                     ", qubit_count=" << qubit_count <<
                                                                     ", creg_count=" << creg_count <<
                                                                     ")");
            }
        }
    }

    for (const auto &kernel : kernels) {
        if (kernel.name == k.name) {
            QL_FATAL("Cannot add kernel. Duplicate kernel name: " << k.name);
        }
    }

    // if sane, now add kernel to list of kernels
    kernels.push_back(k);
}

void quantum_program::add_program(const quantum_program &p) {
    for (auto &k : p.kernels) {
        add(k);
    }
}

void quantum_program::add_if(const quantum_kernel &k, const operation &cond) {
    // phi node
    quantum_kernel kphi1(k.name+"_if", platform, qubit_count, creg_count, breg_count);
    kphi1.set_kernel_type(kernel_type_t::IF_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add(k);

    // phi node
    quantum_kernel kphi2(k.name+"_if_end", platform, qubit_count, creg_count, breg_count);
    kphi2.set_kernel_type(kernel_type_t::IF_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);
}

void quantum_program::add_if(const quantum_program &p, const operation &cond) {
    // phi node
    quantum_kernel kphi1(p.name+"_if", platform, qubit_count, creg_count, breg_count);
    kphi1.set_kernel_type(kernel_type_t::IF_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add_program(p);

    // phi node
    quantum_kernel kphi2(p.name+"_if_end", platform, qubit_count, creg_count, breg_count);
    kphi2.set_kernel_type(kernel_type_t::IF_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);
}

void quantum_program::add_if_else(
    const quantum_kernel &k_if,
    const quantum_kernel &k_else,
    const operation &cond
) {
    quantum_kernel kphi1(k_if.name+"_if"+ to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi1.set_kernel_type(kernel_type_t::IF_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add(k_if);

    // phi node
    quantum_kernel kphi2(k_if.name+"_if"+ to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2.set_kernel_type(kernel_type_t::IF_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);


    // phi node
    quantum_kernel kphi3(k_else.name+"_else" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi3.set_kernel_type(kernel_type_t::ELSE_START);
    kphi3.set_condition(cond);
    kernels.push_back(kphi3);

    add(k_else);

    // phi node
    quantum_kernel kphi4(k_else.name+"_else" + to_string(phi_node_count)+"_end", platform, qubit_count, creg_count, breg_count);
    kphi4.set_kernel_type(kernel_type_t::ELSE_END);
    kphi4.set_condition(cond);
    kernels.push_back(kphi4);

    phi_node_count++;
}

void quantum_program::add_if_else(
    const quantum_program &p_if,
    const quantum_program &p_else,
    const operation &cond
) {
    quantum_kernel kphi1(p_if.name+"_if"+ to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi1.set_kernel_type(kernel_type_t::IF_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add_program(p_if);

    // phi node
    quantum_kernel kphi2(p_if.name+"_if"+ to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2.set_kernel_type(kernel_type_t::IF_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);


    // phi node
    quantum_kernel kphi3(p_else.name+"_else" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi3.set_kernel_type(kernel_type_t::ELSE_START);
    kphi3.set_condition(cond);
    kernels.push_back(kphi3);

    add_program(p_else);

    // phi node
    quantum_kernel kphi4(p_else.name+"_else" + to_string(phi_node_count)+"_end", platform, qubit_count, creg_count, breg_count);
    kphi4.set_kernel_type(kernel_type_t::ELSE_END);
    kphi4.set_condition(cond);
    kernels.push_back(kphi4);

    phi_node_count++;
}

void quantum_program::add_do_while(const quantum_kernel &k, const operation &cond) {
    // phi node
    quantum_kernel kphi1(k.name+"_do_while"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1.set_kernel_type(kernel_type_t::DO_WHILE_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add(k);

    // phi node
    quantum_kernel kphi2(k.name+"_do_while" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi2.set_kernel_type(kernel_type_t::DO_WHILE_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);
    phi_node_count++;
}

void quantum_program::add_do_while(const quantum_program &p, const operation &cond) {
    // phi node
    quantum_kernel kphi1(p.name+"_do_while"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1.set_kernel_type(kernel_type_t::DO_WHILE_START);
    kphi1.set_condition(cond);
    kernels.push_back(kphi1);

    add_program(p);

    // phi node
    quantum_kernel kphi2(p.name+"_do_while" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi2.set_kernel_type(kernel_type_t::DO_WHILE_END);
    kphi2.set_condition(cond);
    kernels.push_back(kphi2);
    phi_node_count++;
}

void quantum_program::add_for(const quantum_kernel &k, UInt iterations) {
    // phi node
    quantum_kernel kphi1(k.name+"_for"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1.set_kernel_type(kernel_type_t::FOR_START);
    kphi1.iterations = iterations;
    kernels.push_back(kphi1);

    add(k);
    kernels.back().iterations = iterations;

    // phi node
    quantum_kernel kphi2(k.name+"_for" + to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2.set_kernel_type(kernel_type_t::FOR_END);
    kernels.push_back(kphi2);
    phi_node_count++;
}

void quantum_program::add_for(const quantum_program &p, UInt iterations) {
    Bool nested_for = false;
    for (auto &k : p.kernels) {
        if (k.type == kernel_type_t::FOR_START) {
            nested_for = true;
        }
    }
    if (nested_for) {
        QL_EOUT("Nested for not yet implemented !");
        throw Exception("Error: Nested for not yet implemented !", false);
    }

    // optimize away if zero iterations
    if (iterations <= 0) {
        return;
    }

    // phi node
    quantum_kernel kphi1(p.name+"_for"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1.set_kernel_type(kernel_type_t::FOR_START);
    kphi1.iterations = iterations;
    kernels.push_back(kphi1);

    // phi node
    quantum_kernel kphi2(p.name, platform, qubit_count, creg_count, breg_count);
    kphi2.set_kernel_type(kernel_type_t::STATIC);
    kernels.push_back(kphi2);

    add_program(p);

    // phi node
    quantum_kernel kphi3(p.name+"_for" + to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi3.set_kernel_type(kernel_type_t::FOR_END);
    kernels.push_back(kphi3);
    phi_node_count++;
}

void quantum_program::set_config_file(const Str &file_name) {
    config_file_name = file_name;
    default_config   = false;
}

void quantum_program::set_platform(const quantum_platform &platform) {
    this->platform = platform;
}

void quantum_program::compile() {
    QL_IOUT("compiling " << name << " ...");
    QL_WOUT("compiling " << name << " ...");
    if (kernels.empty()) {
        QL_FATAL("compiling a program with no kernels !");
    }

    // from here on front-end passes

    // writer pass of the initial qasm file (program.qasm)
    write_qasm(this, platform, "initialqasmwriter");

    // rotation_optimize pass
    rotation_optimize(this, platform, "rotation_optimize");

    // decompose_toffoli pass
    decompose_toffoli(this, platform, "decompose_toffoli");

    // clifford optimize
    clifford_optimize(this, platform, "clifford_prescheduler");

    // prescheduler pass
    schedule(this, platform, "prescheduler");

    // clifford optimize
    clifford_optimize(this, platform, "clifford_postscheduler");

    // writer pass of the scheduled qasm file (program_scheduled.qasm)
    write_qasm(this, platform, "scheduledqasmwriter");

    // backend passes
    QL_DOUT("eqasm_compiler_name: " << eqasm_compiler_name);
    if (!needs_backend_compiler) {
        QL_WOUT("The eqasm compiler attribute indicated that no backend passes are needed.");
        return;
    } if (!backend_compiler) {
        QL_EOUT("No known eqasm compiler has been specified in the configuration file.");
        return;
    } else {
        QL_DOUT("About to call backend_compiler->compile for " << eqasm_compiler_name);
        backend_compiler->compile(this, platform);
        QL_DOUT("Returned from call backend_compiler->compile for " << eqasm_compiler_name);
    }

    // generate sweep_points file
    write_sweep_points(this, platform, "write_sweep_points");

    QL_IOUT("compilation of program '" << name << "' done.");
}

void quantum_program::compile_modular() {
    QL_IOUT("compiling " << name << " ...");
    QL_WOUT("compiling " << name << " ...");
    if (kernels.empty()) {
        QL_FATAL("compiling a program with no kernels !");
    }

    //constuct compiler
    std::unique_ptr<quantum_compiler> compiler(new quantum_compiler("Hard Coded Compiler"));

    //add passes
    ///@note-rn: WriterPass needs Reader pass to recreate the subciruits! ==> However, then Reader needs to be used to recreate the subcircuits. However, if I do that tests will fail because the harddware configuration file is in synq with qasm reader and tests (error: unrecognized instr prepz)
    compiler->addPass("Writer", "initialqasmwriter");
    compiler->addPass("RotationOptimizer", "rotation_optimize");
    compiler->addPass("DecomposeToffoli", "decompose_toffoli");
    compiler->addPass("CliffordOptimize", "clifford_prescheduler");
    compiler->addPass("Scheduler", "prescheduler");
    compiler->addPass("CliffordOptimize", "clifford_postscheduler");
    compiler->addPass("Writer", "scheduledqasmwriter");

    // backend passes
    QL_DOUT("Calling backend compiler passes for eqasm_compiler_name: " << eqasm_compiler_name);
    if (eqasm_compiler_name.empty()) {
        QL_FATAL("eqasm compiler name must be specified in the hardware configuration file !");
    } else if (eqasm_compiler_name == "none" || eqasm_compiler_name == "qx") {
        QL_WOUT("The eqasm compiler attribute indicated that no backend passes are needed.");
    } else if (eqasm_compiler_name == "cc_light_compiler") {
        // from here CCL backend starts
        compiler->addPass("CCLPrepCodeGeneration", "ccl_prep_code_generation");
        compiler->addPass("CCLDecomposePreSchedule", "ccl_decompose_pre_schedule");
        compiler->addPass("WriteQuantumSim", "write_quantumsim_script_unmapped");
        compiler->addPass("CliffordOptimize", "clifford_premapper");
        compiler->addPass("Map", "mapper");
        compiler->addPass("CliffordOptimize", "clifford_postmapper");
        // compiler->addPass("CommuteVariation", "commute_variation");
        compiler->addPass("RCSchedule", "rcscheduler");
        compiler->addPass("LatencyCompensation", "ccl_latency_compensation");
        compiler->addPass("InsertBufferDelays", "ccl_insert_buffer_delays");
        compiler->addPass("CCLDecomposePostSchedule", "ccl_decompose_post_schedule");
        compiler->addPass("WriteQuantumSim", "write_quantumsim_script_mapped");
        compiler->addPass("Writer", "lastqasmwriter");
        compiler->addPass("QisaCodeGeneration", "qisa_code_generation");
        ///@note-rn: Calling the backend like this is equivalend to calling passes individually as above.
        //compiler->addPass("BackendCompiler");
        //compiler->setPassOption("BackendCompiler", "eqasm_compiler_name", eqasm_compiler_name);
        QL_DOUT("Returned from call backend_compiler->compile for " << eqasm_compiler_name);
    } else if (eqasm_compiler_name == "eqasm_backend_cc") {
        compiler->addPass("BackendCompiler");
        compiler->setPassOption("BackendCompiler", "eqasm_compiler_name", "eqasm_backend_cc");
    } else {
        QL_FATAL("the '" << eqasm_compiler_name << "' eqasm compiler backend is not suported !");
    }

    //compile with program
    compiler->compile(this);

    QL_IOUT("compilation of program '" << name << "' done.");

    compiler.reset();
}

void quantum_program::print_interaction_matrix() const {
    QL_IOUT("printing interaction matrix...");

    for (auto k : kernels) {
        InteractionMatrix imat( k.get_circuit(), qubit_count);
        Str mstr = imat.getString();
        std::cout << mstr << std::endl;
    }
}

void quantum_program::write_interaction_matrix() const {
    for (auto k : kernels) {
        InteractionMatrix imat(k.get_circuit(), qubit_count);
        Str mstr = imat.getString();

        Str fname = options::get("output_dir") + "/" + k.get_name() + "InteractionMatrix.dat";
        QL_IOUT("writing interaction matrix to '" << fname << "' ...");
        OutFile(fname).write(mstr);
    }
}

void quantum_program::set_sweep_points(const Real *swpts, UInt size) {
    sweep_points.clear();
    for (UInt i = 0; i < size; ++i) {
        sweep_points.push_back(swpts[i]);
    }
}

Vec<quantum_kernel> &quantum_program::get_kernels() {
    return kernels;
}

const Vec<quantum_kernel> &quantum_program::get_kernels() const {
    return kernels;
}

} // namespace ql
