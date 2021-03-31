/** \file
 * Quantum program abstraction implementation.
 */

#include "ql/ir/program.h"

#include "ql/utils/filesystem.h"
#include "ql/com/options/options.h"
#include "compiler.h"
#include "interactionMatrix.h"
#include "scheduler.h"

static unsigned long phi_node_count = 0;    // FIXME: number across quantum_program instances

namespace ql {
namespace ir {

using namespace utils;

/**
 * @brief   Quantum program constructor
 * @param   n   Name of the program
 */
Program::Program(const Str &n) : name(n) {
    platformInitialized = false;
    QL_DOUT("Constructor for quantum_program:  " << n);
}

Program::Program(
    const Str &n,
    const plat::PlatformRef &platf,
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
    platformInitialized = true;

    if (qubit_count > platform->qubit_number) {
        QL_FATAL("number of qubits requested in program '" + to_string(qubit_count) + "' is greater than the qubits available in platform '" + to_string(platform->qubit_number) + "'" );
    }

    // report/write_qasm initialization
    report_init(*this, platform);
}

void Program::add(const KernelRef &k) {
    // check sanity of supplied qubit/classical operands for each gate
    Circuit &kc = k->get_circuit();
    for (auto &g : kc) {
        auto &gate_operands = g->operands;
        auto &gname = g->name;
        auto gtype = g->type();
        for (auto &op : gate_operands) {
            if (
                ((gtype == GateType::CLASSICAL) && (op >= creg_count)) ||
                ((gtype != GateType::CLASSICAL) && (op >= qubit_count))
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
        if (kernel->name == k->name) {
            QL_FATAL("Cannot add kernel. Duplicate kernel name: " << k->name);
        }
    }

    // if sane, now add kernel to list of kernels
    kernels.add(k);
}

void Program::add_program(const ProgramRef &p) {
    for (auto &k : p->kernels) {
        add(k);
    }
}

void Program::add_if(const KernelRef &k, const ClassicalOperation &cond) {
    // phi node
    auto kphi1 = KernelRef::make(k->name+"_if", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::IF_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add(k);

    // phi node
    auto kphi2 = KernelRef::make(k->name+"_if_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::IF_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);
}

void Program::add_if(const ProgramRef &p, const ClassicalOperation &cond) {
    // phi node
    auto kphi1 = KernelRef::make(p->name+"_if", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::IF_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add_program(p);

    // phi node
    auto kphi2 = KernelRef::make(p->name+"_if_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::IF_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);
}

void Program::add_if_else(
    const KernelRef &k_if,
    const KernelRef &k_else,
    const ClassicalOperation &cond
) {
    auto kphi1 = KernelRef::make(k_if->name+"_if"+ to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::IF_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add(k_if);

    // phi node
    auto kphi2 = KernelRef::make(k_if->name+"_if"+ to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::IF_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);


    // phi node
    auto kphi3 = KernelRef::make(k_else->name+"_else" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi3->set_kernel_type(KernelType::ELSE_START);
    kphi3->set_condition(cond);
    kernels.add(kphi3);

    add(k_else);

    // phi node
    auto kphi4 = KernelRef::make(k_else->name+"_else" + to_string(phi_node_count)+"_end", platform, qubit_count, creg_count, breg_count);
    kphi4->set_kernel_type(KernelType::ELSE_END);
    kphi4->set_condition(cond);
    kernels.add(kphi4);

    phi_node_count++;
}

void Program::add_if_else(
    const ProgramRef &p_if,
    const ProgramRef &p_else,
    const ClassicalOperation &cond
) {
    auto kphi1 = KernelRef::make(p_if->name+"_if"+ to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::IF_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add_program(p_if);

    // phi node
    auto kphi2 = KernelRef::make(p_if->name+"_if"+ to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::IF_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);


    // phi node
    auto kphi3 = KernelRef::make(p_else->name+"_else" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi3->set_kernel_type(KernelType::ELSE_START);
    kphi3->set_condition(cond);
    kernels.add(kphi3);

    add_program(p_else);

    // phi node
    auto kphi4 = KernelRef::make(p_else->name+"_else" + to_string(phi_node_count)+"_end", platform, qubit_count, creg_count, breg_count);
    kphi4->set_kernel_type(KernelType::ELSE_END);
    kphi4->set_condition(cond);
    kernels.add(kphi4);

    phi_node_count++;
}

void Program::add_do_while(const KernelRef &k, const ClassicalOperation &cond) {
    // phi node
    auto kphi1 = KernelRef::make(k->name+"_do_while"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::DO_WHILE_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add(k);

    // phi node
    auto kphi2 = KernelRef::make(k->name+"_do_while" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::DO_WHILE_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);
    phi_node_count++;
}

void Program::add_do_while(const ProgramRef &p, const ClassicalOperation &cond) {
    // phi node
    auto kphi1 = KernelRef::make(p->name+"_do_while"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::DO_WHILE_START);
    kphi1->set_condition(cond);
    kernels.add(kphi1);

    add_program(p);

    // phi node
    auto kphi2 = KernelRef::make(p->name+"_do_while" + to_string(phi_node_count), platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::DO_WHILE_END);
    kphi2->set_condition(cond);
    kernels.add(kphi2);
    phi_node_count++;
}

void Program::add_for(const KernelRef &k, UInt iterations) {
    // phi node
    auto kphi1 = KernelRef::make(k->name+"_for"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::FOR_START);
    kphi1->iterations = iterations;
    kernels.add(kphi1);

    add(k);
    kernels.back()->iterations = iterations;

    // phi node
    auto kphi2 = KernelRef::make(k->name+"_for" + to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::FOR_END);
    kernels.add(kphi2);
    phi_node_count++;
}

void Program::add_for(const ProgramRef &p, UInt iterations) {
    Bool nested_for = false;
//     for (auto &k : p.kernels) {
//         if (k.type == kernel_type_t::FOR_START) {
//             nested_for = true;
//         }
//     }
    if (nested_for) {
        QL_EOUT("Nested for not yet implemented !");
        throw Exception("Error: Nested for not yet implemented !", false);
    }

    // optimize away if zero iterations
    if (iterations <= 0) {
        return;
    }

    // phi node
    auto kphi1 = KernelRef::make(p->name+"_for"+ to_string(phi_node_count) +"_start", platform, qubit_count, creg_count, breg_count);
    kphi1->set_kernel_type(KernelType::FOR_START);
    kphi1->iterations = iterations;
    kernels.add(kphi1);

    // phi node
    auto kphi2 = KernelRef::make(p->name, platform, qubit_count, creg_count, breg_count);
    kphi2->set_kernel_type(KernelType::STATIC);
    kernels.add(kphi2);

    add_program(p);

    // phi node
    auto kphi3 = KernelRef::make(p->name+"_for" + to_string(phi_node_count) +"_end", platform, qubit_count, creg_count, breg_count);
    kphi3->set_kernel_type(KernelType::FOR_END);
    kernels.add(kphi3);
    phi_node_count++;
}

void Program::set_config_file(const Str &file_name) {
    config_file_name = file_name;
    default_config   = false;
}

void Program::set_platform(const plat::PlatformRef &platform) {
    this->platform = platform;
}

std::string dirnameOf(const std::string& fname)
{
     size_t pos = fname.find_last_of("\\/");
     return (std::string::npos == pos) ? "" : fname.substr(0, pos)+"/";
}

void Program::compile() {
    QL_IOUT("compiling " << name << " ...");
    QL_WOUT("compiling " << name << " ...");
    if (kernels.empty()) {
        QL_FATAL("compiling a program with no kernels !");
    }

    // Retrieve the path to the platform configuration file.
    // This is needed below to circumvent the hardcoding of the compiler configuration file
    // when this legacy ::compile method is used.
    // NOTE: For the use of 'compilerCfgPath' below to work, it is assumed the compiler configuration file
    //       is located in the same folder as the platform configuration file. 
    std::string compilerCfgPath = dirnameOf(platform->configuration_file_name);

    //constuct compiler
    std::unique_ptr<quantum_compiler> compiler(new quantum_compiler("Hard Coded Compiler"));

    // backend passes
    QL_DOUT("Calling backend compiler passes for eqasm_compiler_name: " << platform->eqasm_compiler_name);
    if (platform->eqasm_compiler_name.empty()) {
        QL_FATAL("eqasm compiler name must be specified in the hardware configuration file !");
    } else if (platform->eqasm_compiler_name == "none" || platform->eqasm_compiler_name == "qx") {
        QL_WOUT("The eqasm compiler attribute indicated that no backend passes are needed.");
        compiler->loadPassesFromConfigFile("QX_compiler", compilerCfgPath+"qx_compiler_cfg.json");
    } else if (platform->eqasm_compiler_name == "cc_light_compiler") {
        compiler->loadPassesFromConfigFile("CCLight_compiler", compilerCfgPath+"cclight_compiler_cfg.json");
        QL_DOUT("Returned from call backend_compiler->compile for " << platform->eqasm_compiler_name);
    } else if (platform->eqasm_compiler_name == "eqasm_backend_cc") {
        compiler->loadPassesFromConfigFile("CC_compiler", compilerCfgPath+"cc_compiler_cfg.json");
    } else {
        QL_FATAL("the '" << platform->eqasm_compiler_name << "' eqasm compiler backend is not suported !");
    }

    //compile with program
    compiler->compile(ProgramRef::make(*this));

    QL_IOUT("compilation of program '" << name << "' done.");

    compiler.reset();
}

void Program::print_interaction_matrix() const {
    QL_IOUT("printing interaction matrix...");

    for (const auto &k : kernels) {
        InteractionMatrix imat(k->get_circuit(), qubit_count);
        Str mstr = imat.getString();
        std::cout << mstr << std::endl;
    }
}

void Program::write_interaction_matrix() const {
    for (const auto &k : kernels) {
        InteractionMatrix imat(k->get_circuit(), qubit_count);
        Str mstr = imat.getString();

        Str fname = com::options::get("output_dir") + "/" + k->get_name() + "InteractionMatrix.dat";
        QL_IOUT("writing interaction matrix to '" << fname << "' ...");
        OutFile(fname).write(mstr);
    }
}

void Program::set_sweep_points(const Real *swpts, UInt size) {
    sweep_points.clear();
    for (UInt i = 0; i < size; ++i) {
        sweep_points.push_back(swpts[i]);
    }
}

KernelRefs &Program::get_kernels() {
    return kernels;
}

const KernelRefs &Program::get_kernels() const {
    return kernels;
}

} // namespace ir
} // namespace ql
